#include <Python.h>

/* Identity */

PyDoc_STRVAR(Identity__doc__,
"A callable which always returns its first argument."
);

static PyObject *
Identity__call__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) == 0) {
        PyErr_Format(PyExc_TypeError, "expected at least one argument");
        return NULL;
    }

    PyObject *retval = PyTuple_GET_ITEM(args, 0);
    Py_INCREF(retval);
    return retval;
}

static PyTypeObject Identity_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b._functools.Identity",   /* tp_name */
    sizeof(PyObject),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    Identity__call__,          /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    Identity__doc__,           /* tp_doc */
};

/* Noop */

PyDoc_STRVAR(Noop__doc__,
"A do-nothing callable."
);

static PyObject *
Noop__call__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_RETURN_NONE;
}

static PyTypeObject Noop_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b._functools.Noop",       /* tp_name */
    sizeof(PyObject),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    Noop__call__,              /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    Noop__doc__,               /* tp_doc */
};

PyDoc_STRVAR(Module__doc__,
"TODO module __doc__");

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "b._functools",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__functools(void)
{
    PyObject *module = PyModule_Create(&Module);
    if (module == NULL)
        return NULL;

    /* identity */

    if (PyType_Ready(&Identity_type) < 0)
        return NULL;

    PyObject *identity = PyObject_New(PyObject, &Identity_type);
    if (identity == NULL)
        return NULL;

    Py_INCREF(identity);

    PyModule_AddObject(module, "identity", identity);

    /* noop */

    if (PyType_Ready(&Noop_type) < 0)
        return NULL;

    PyObject *noop = PyObject_New(PyObject, &Noop_type);
    if (noop == NULL)
        return NULL;

    Py_INCREF(noop);

    PyModule_AddObject(module, "noop", noop);

    return module;
};
