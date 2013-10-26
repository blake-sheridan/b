#include <Python.h>

#include "lazy.h"

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Property;

PyDoc_STRVAR(__doc__,
"TODO property __doc__");

static PyObject *
__new__(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    return new(type, args, kwargs);
}

static void
__del__(PyObject *self)
{
    dealloc(self);
}

static PyObject *
__doc__getter(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__doc__");
}

static PyObject *
__name__getter(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__name__");
}

static PyObject *
__qualname__getter(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->function, "__qualname__");
}

static PyGetSetDef getset[] = {
    {"__doc__",      (getter)__doc__getter},
    {"__name__",     (getter)__name__getter},
    {"__qualname__", (getter)__qualname__getter},
    {0}
};

static PyObject *
__get__(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    return get((Cache *)self, instance);
}

static int
__set__(PyObject *self, PyObject *instance, PyObject *value)
{
    return set(self, instance, value);
}

PyTypeObject PropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "property",                /* tp_name */
    sizeof(Property),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)__del__,       /* tp_dealloc */
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
    __doc__,                   /* tp_doc */
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
    (descrgetfunc)__get__,     /* tp_descr_get */
    (descrsetfunc)__set__,     /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)__new__,          /* tp_new */
};
