#include <Python.h>

#include "lazy.h"

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Property;

/* property */

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
property_get(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    return get((Cache *)self, instance);
}

PyDoc_STRVAR(property_doc,
"TODO property __doc__");

PyTypeObject PropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "property",                /* tp_name */
    sizeof(Property),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)dealloc,       /* tp_dealloc */
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
    (descrgetfunc)property_get,/* tp_descr_get */
    (descrsetfunc)set,         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)new,              /* tp_new */
};
