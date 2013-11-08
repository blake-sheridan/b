#include <Python.h>

#include "memoizer.h"

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Memoizer *memoizer;
} LazyProperty;

PyDoc_STRVAR(LazyProperty__doc__,
"TODO property __doc__");

extern PyTypeObject MemoizerType;

static PyObject *
LazyProperty__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     Py_TYPE(function)->tp_name);
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(function);

    ((LazyProperty *)self)->function = function;
    ((LazyProperty *)self)->memoizer = NULL;

    return self;
}

static void
LazyProperty__del__(PyObject *self)
{
    Py_DECREF(((LazyProperty *)self)->function);
    Py_XDECREF(((LazyProperty *)self)->memoizer);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
LazyProperty__doc__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__doc__");
}

static PyObject *
LazyProperty__name__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__name__");
}

static PyObject *
LazyProperty__qualname__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__qualname__");
}

static PyGetSetDef getset[] = {
    {"__doc__",      LazyProperty__doc__getter},
    {"__name__",     LazyProperty__name__getter},
    {"__qualname__", LazyProperty__qualname__getter},
    {0}
};

static PyObject *
LazyProperty__get__(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    LazyProperty *this = (LazyProperty *)self;

    if (this->memoizer == NULL) {
        this->memoizer = Memoizer_new(this->function);
        if (this->memoizer == NULL)
            return NULL;
    }

    return Memoizer_get(this->memoizer, instance);
}

static int
LazyProperty__set__(PyObject *self, PyObject *instance, PyObject *value)
{
    LazyProperty *this = (LazyProperty *)self;

    if (this->memoizer == NULL) {
        this->memoizer = Memoizer_new(this->function);
        if (this->memoizer == NULL)
            return -1;
    }

    return Memoizer_assign(this->memoizer, instance, value);
}

PyTypeObject LazyPropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "property",                /* tp_name */
    sizeof(LazyProperty),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    LazyProperty__del__,       /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    LazyProperty__doc__,       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    getset,                    /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    LazyProperty__get__,       /* tp_descr_get */
    LazyProperty__set__,       /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    LazyProperty__new__,       /* tp_new */
};

/* Module */

PyDoc_STRVAR(Module__doc__,
"TODO module __doc__");

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "b._types",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__types(void)
{
    PyObject *module = PyModule_Create(&Module);

    if (module == NULL)
        return NULL;

    /* LazyProperty */

    if (PyType_Ready(&LazyPropertyType) < 0)
        return NULL;

    Py_INCREF(&LazyPropertyType);

    PyModule_AddObject(module, "lazyproperty", (PyObject *)&LazyPropertyType);

    /* Memoizer */

    if (PyType_Ready(&MemoizerType) < 0)
        return NULL;

    Py_INCREF(&MemoizerType);

    PyModule_AddObject(module, "Memoizer", (PyObject *)&MemoizerType);

    return module;
};
