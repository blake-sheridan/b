#include <Python.h>

extern PyTypeObject LazyPropertyType;

/* Module */

PyDoc_STRVAR(Module__doc__,
"TODO module __doc__");

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "_lazy",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__lazy(void)
{
    PyObject *module = PyModule_Create(&Module);

    if (module != NULL) {
        if (PyType_Ready(&LazyPropertyType) < 0)
            return NULL;

        Py_INCREF(&LazyPropertyType);

        PyModule_AddObject(module, "property", (PyObject *)&LazyPropertyType);
    }

    return module;
};
