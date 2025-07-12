#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { runBenchmark } = require('./wrt');

function printUsage() {
  console.log(`Uso: node cli.js [URL | -f archivo] -r threads -t duracion -w timeout [--insecure] [-x]`);
  console.log(`Ejemplos:`);
  console.log(`  wrt-js https://example.com -r 10 -t 30 -w 5000`);
  console.log(`  wrt-js -f urls.txt -r 4 -t 15 -w 3000 --insecure`);
  console.log(`  wrt-js https://localhost -r 4 -t 10 -w 1000 -x`);
}

function parseArgs(argv) {
  if (argv.length < 3) {
    printUsage();
    process.exit(1);
  }

  const config = {
    urls: [],
    threads: 1,
    duration: 10,
    timeout: 5000,
    insecure: false,
    ignore_url: false,
  };

  let i = 2;
  while (i < argv.length) {
    const arg = argv[i];
    if (arg === '-r' && argv[i + 1]) { config.threads = parseInt(argv[++i]); }
    else if (arg === '-t' && argv[i + 1]) { config.duration = parseInt(argv[++i]); }
    else if (arg === '-w' && argv[i + 1]) { config.timeout = parseInt(argv[++i]); }
    else if (arg === '--insecure') { config.insecure = true; }
    else if (arg === '-x') { config.ignore_url = true; }
    else if (arg === '-f' && argv[i + 1]) {
      const filePath = argv[++i];
      if (!fs.existsSync(filePath)) {
        console.error(`Archivo no encontrado: ${filePath}`);
        process.exit(1);
      }
      const content = fs.readFileSync(filePath, 'utf-8');
      config.urls = content.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    } else if (arg.startsWith('http://') || arg.startsWith('https://') || arg.startsWith('rtmp://')) {
      config.urls.push(arg);
    } else {
      console.log(`Argumento inválido: ${arg}`);
      printUsage();
      process.exit(1);
    }
    i++;
  }

  if (config.urls.length === 0) {
    console.error('Debe especificar al menos una URL o usar -f archivo');
    process.exit(1);
  }

  return config;
}

function printProgressBar(elapsed, total) {
  const width = 30;
  const filled = Math.floor((elapsed * width) / total);
  const bar = '[' + '#'.repeat(filled) + '.'.repeat(width - filled) + ']';
  const percent = Math.floor((elapsed * 100) / total);
  process.stdout.write(`\r${bar} ${percent}% (${elapsed}s/${total}s)`);
}

async function runSingleBenchmark(url, config) {
  console.log(`\nIniciando prueba para: ${url}`);
  console.log(`Threads: ${config.threads}, Duración: ${config.duration}s, Timeout: ${config.timeout}ms`);
  if (config.insecure) console.log(`⚠️ Ignorando verificación SSL (--insecure)`);
  if (config.ignore_url) console.log(`⚠️ Modo simulación activado (-x), sin peticiones reales`);

  if (config.ignore_url) {
    // Simular carga sin requests reales
    let requests = 0;
    const startTime = Date.now();
    return new Promise(resolve => {
      const interval = setInterval(() => {
        requests += config.threads;
      }, 1000);
      const timer = setTimeout(() => {
        clearInterval(interval);
        resolve({
          requests,
          errors: 0,
          timeouts: 0,
          avg_latency: 0,
          min_latency: 0,
          max_latency: 0,
        });
      }, config.duration * 1000);

      // Barra de progreso
      let elapsed = 0;
      const progressInterval = setInterval(() => {
        elapsed++;
        printProgressBar(elapsed, config.duration);
        if (elapsed >= config.duration) {
          clearInterval(progressInterval);
          process.stdout.write('\n');
        }
      }, 1000);
    });
  }

  // Barra de progreso + benchmark real
  let elapsed = 0;
  const progressInterval = setInterval(() => {
    elapsed++;
    printProgressBar(elapsed, config.duration);
  }, 1000);

  const result = await runBenchmark({
    url,
    threads: config.threads,
    duration: config.duration,
    timeout: config.timeout,
    insecure: config.insecure,
  });

  clearInterval(progressInterval);
  printProgressBar(config.duration, config.duration);
  process.stdout.write('\n');

  return result;
}

function saveCSV(results, file = 'wrt_results.csv') {
  const headers = 'timestamp,url,threads,duration,requests,errors,timeouts,avg_latency,min_latency,max_latency\n';
  const exists = fs.existsSync(file);
  const lines = results.map(r => {
    return [
      Date.now(),
      r.url,
      r.threads,
      r.duration,
      r.requests,
      r.errors,
      r.timeouts,
      r.avg_latency,
      r.min_latency,
      r.max_latency
    ].join(',');
  }).join('\n');

  if (!exists) {
    fs.writeFileSync(file, headers + lines + '\n', { flag: 'a' });
  } else {
    fs.writeFileSync(file, lines + '\n', { flag: 'a' });
  }
}

(async () => {
  const config = parseArgs(process.argv);

  let allResults = [];

  for (const url of config.urls) {
    const res = await runSingleBenchmark(url, {
      threads: config.threads,
      duration: config.duration,
      timeout: config.timeout,
      insecure: config.insecure,
      ignore_url: config.ignore_url,
    });
    allResults.push({ url, threads: config.threads, duration: config.duration, ...res });

    console.log(`\n--- Resultados para ${url} ---`);
    console.log(`Requests: ${res.requests}`);
    console.log(`Errores: ${res.errors}`);
    console.log(`Timeouts: ${res.timeouts}`);
    console.log(`Latencia promedio: ${res.avg_latency} s`);
    console.log(`Latencia min: ${res.min_latency} s | max: ${res.max_latency} s`);
  }

  saveCSV(allResults);
  console.log(`\n✅ Resultados guardados en wrt_results.csv`);
})();
