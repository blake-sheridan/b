#include <Python.h>

#include "funcobject.h"

#define ENTRIES_SIZE 512

typedef struct {
    PyObject *instance;
    PyObject *value;
} Entry;

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Entry (*entries)[];
} LazyProperty;

static PyObject *
lazyproperty_new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    PyObject *self;
    PyObject *function;

    if (PyTuple_GET_SIZE(args) != 1) {
        PyErr_SetString(PyExc_TypeError,
                       "type 'lazyproperty' takes a single argument");
        return NULL;
    }

    function = PyTuple_GET_ITEM(args, 0);
    if (!PyFunction_Check(function)) {
        PyErr_SetString(PyExc_TypeError,
                        "expecting function");
        return NULL;
    }

    self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    LazyProperty *lp = (LazyProperty *)self;

    lp->function = function;
    Py_INCREF(function);

    lp->entries = NULL;

    return self;
}

static void
lazyproperty_dealloc(PyObject *self)
{
    LazyProperty *lp = (LazyProperty *)self;

    Py_XDECREF(lp->function);

    if (lp->entries != NULL) {
        PyMem_Free(lp->entries);
        lp->entries = NULL;
    }

    self->ob_type->tp_free(self);
}

static PyObject *
lazyproperty_get_doc(LazyProperty *lp)
{
    return PyObject_GetAttrString(lp->function, "__doc__");
}

static PyObject *
lazyproperty_get_name(LazyProperty *lp)
{
    return PyObject_GetAttrString(lp->function, "__name__");
}

static PyObject *
lazyproperty_get_qualname(LazyProperty *lp)
{
    return PyObject_GetAttrString(lp->function, "__qualname__");
}

static PyGetSetDef lazyproperty_getset[] = {
    {"__doc__",      (getter)lazyproperty_get_doc},
    {"__name__",     (getter)lazyproperty_get_name},
    {"__qualname__", (getter)lazyproperty_get_qualname},
    {0}
};

static PyObject *
lazyproperty_descr_get(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    LazyProperty *lp = (LazyProperty *)self;

    if (lp->entries == NULL) {
        lp->entries = PyMem_Malloc(ENTRIES_SIZE * sizeof(Entry));

        if (lp->entries == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    long index = (long)instance % ENTRIES_SIZE;
    Entry entry = (*lp->entries)[index];
    PyObject *value;

    if (entry.instance == instance) {
        value = entry.value;
    } else {
        value = PyObject_CallFunctionObjArgs(lp->function, instance, NULL);
        if (value == NULL) {
            return NULL;
        }

        (*lp->entries)[index].instance = instance;
        (*lp->entries)[index].value = value;
    }

    Py_INCREF(value);

    return value;
}

static PyObject *
lazyproperty_descr_set(PyObject *self, PyObject *instance, PyObject *value)
{
    PyErr_SetString(PyExc_NotImplementedError, "lazyproperty.__set__");
    return NULL;
}

PyDoc_STRVAR(lazyproperty_doc,
"TODO lazyproperty __doc__");

PyTypeObject LazyPropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "lazyproperty",            /* tp_name */
    sizeof(LazyProperty),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)lazyproperty_dealloc,   /* tp_dealloc */
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
    lazyproperty_doc,          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    lazyproperty_getset,       /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    (descrgetfunc)lazyproperty_descr_get,    /* tp_descr_get */
    (descrsetfunc)lazyproperty_descr_set,    /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)lazyproperty_new, /* tp_new */
};
