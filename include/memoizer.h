#ifndef MEMOIZER_H_
#define MEMOIZER_H_

#include <Python.h>

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Py_ssize_t size;
    Py_ssize_t usable;
    Entry (*entries)[1];
} Memoizer;

Memoizer *
Memoizer_new(PyObject *function);

PyObject *
Memoizer_get(Memoizer *memoizer, PyObject *key);

int
Memoizer_assign(Memoizer *memoizer, PyObject *key, PyObject *value);

#endif
