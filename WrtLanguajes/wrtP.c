#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "wrt.h"

// Wrapper para la función wrt_start
static PyObject* py_wrt_start(PyObject* self, PyObject* args) {
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    wrt_config_t config;
    wrt_init_config(&config);
    strncpy(config.url, url, sizeof(config.url));
    wrt_start(&config);

    Py_RETURN_NONE;
}

// Métodos del módulo
static PyMethodDef WrtMethods[] = {
    {"start", py_wrt_start, METH_VARARGS, "Inicia WRT con la URL dada"},
    {NULL, NULL, 0, NULL}
};

// Definición del módulo
static struct PyModuleDef wrtmodule = {
    PyModuleDef_HEAD_INIT,
    "wrt",
    "Módulo Python para WRT",
    -1,
    WrtMethods
};

// Inicialización del módulo
PyMODINIT_FUNC PyInit_wrt(void) {
    return PyModule_Create(&wrtmodule);
}
