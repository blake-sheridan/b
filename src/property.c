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
} Property;

static PyObject *
property_new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    PyObject *self;
    PyObject *function;

    if (PyTuple_GET_SIZE(args) != 1) {
        PyErr_SetString(PyExc_TypeError,
                       "type 'lazy.property' takes a single argument");
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

    Py_INCREF(function);

    Property *this = (Property*)self;

    this->function = function;
    this->entries = NULL;

    return self;
}

static void
property_dealloc(PyObject *self)
{
    Property *this = (Property*)self;

    Py_XDECREF(this->function);

    if (this->entries != NULL) {
        PyMem_Free(this->entries);
        this->entries = NULL;
    }

    self->ob_type->tp_free(self);
}

static PyObject *
property_get_doc(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__doc__");
}

static PyObject *
property_get_name(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__name__");
}

static PyObject *
property_get_qualname(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__qualname__");
}

static PyGetSetDef property_getset[] = {
    {"__doc__",      (getter)property_get_doc},
    {"__name__",     (getter)property_get_name},
    {"__qualname__", (getter)property_get_qualname},
    {0}
};

static PyObject *
property_descr_get(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    Property *this = (Property *)self;

    if (this->entries == NULL) {
        this->entries = PyMem_Malloc(ENTRIES_SIZE * sizeof(Entry));

        if (this->entries == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    long index = (long)instance % ENTRIES_SIZE;
    Entry entry = (*this->entries)[index];
    PyObject *value;

    if (entry.instance == instance) {
        value = entry.value;
    } else {
        value = PyObject_CallFunctionObjArgs(this->function, instance, NULL);
        if (value == NULL) {
            return NULL;
        }

        (*this->entries)[index].instance = instance;
        (*this->entries)[index].value = value;
    }

    Py_INCREF(value);

    return value;
}

static PyObject *
property_descr_set(PyObject *self, PyObject *instance, PyObject *value)
{
    PyErr_SetString(PyExc_NotImplementedError, "property.__set__");
    return NULL;
}

PyDoc_STRVAR(property_doc,
"TODO property __doc__");

PyTypeObject PropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "property",                /* tp_name */
    sizeof(Property),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)property_dealloc,   /* tp_dealloc */
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
    property_doc,              /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    property_getset,           /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    (descrgetfunc)property_descr_get,    /* tp_descr_get */
    (descrsetfunc)property_descr_set,    /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)property_new,     /* tp_new */
};
