const https = require('https');
const http = require('http');
const { performance } = require('perf_hooks');

function makeRequest(url, timeout, insecure = false) {
  return new Promise((resolve, reject) => {
    const mod = url.startsWith('https') ? https : http;
    const start = performance.now();

    const options = {
      timeout,
      rejectUnauthorized: !insecure,
    };

    const req = mod.get(url, options, (res) => {
      res.on('data', () => {}); // ignorar cuerpo
      res.on('end', () => {
        const latency = (performance.now() - start) / 1000;
        resolve({ status: res.statusCode, latency });
      });
    });

    req.on('error', (err) => {
      reject(err);
    });

    req.on('timeout', () => {
      req.destroy();
      reject(new Error('timeout'));
    });
  });
}

async function runBenchmark({ url, threads = 5, duration = 10, timeout = 5000, insecure = false }) {
  const stats = {
    requests: 0,
    errors: 0,
    timeouts: 0,
    latencies: [],
  };

  const endTime = Date.now() + duration * 1000;

  async function worker() {
    while (Date.now() < endTime) {
      try {
        const { latency, status } = await makeRequest(url, timeout, insecure);
        // Sólo contar respuestas 2xx y 3xx como OK
        if (status >= 200 && status < 400) {
          stats.requests++;
          stats.latencies.push(latency);
        } else {
          stats.errors++;
        }
      } catch (err) {
        if (err.message === 'timeout') {
          stats.timeouts++;
        } else {
          stats.errors++;
        }
      }
    }
  }

  const workers = [];
  for (let i = 0; i < threads; i++) {
    workers.push(worker());
  }
  await Promise.all(workers);

  // Calcular latencias
  const totalLat = stats.latencies.reduce((a, b) => a + b, 0);
  const avgLatency = stats.latencies.length ? totalLat / stats.latencies.length : 0;
  const minLatency = stats.latencies.length ? Math.min(...stats.latencies) : 0;
  const maxLatency = stats.latencies.length ? Math.max(...stats.latencies) : 0;

  return {
    requests: stats.requests,
    errors: stats.errors,
    timeouts: stats.timeouts,
    avg_latency: avgLatency.toFixed(3),
    min_latency: minLatency.toFixed(3),
    max_latency: maxLatency.toFixed(3),
  };
}

module.exports = { runBenchmark };
