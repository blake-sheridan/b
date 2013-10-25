#include <Python.h>

extern PyTypeObject MemoizerType;
extern PyTypeObject PropertyType;

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
        if (PyType_Ready(&PropertyType) < 0)
            return NULL;

        Py_INCREF(&PropertyType);

        PyModule_AddObject(module, "property", (PyObject *)&PropertyType);

        if (PyType_Ready(&MemoizerType) < 0)
            return NULL;

        Py_INCREF(&MemoizerType);

        PyModule_AddObject(module, "Memoizer", (PyObject *)&MemoizerType);
    }

    return module;
};
