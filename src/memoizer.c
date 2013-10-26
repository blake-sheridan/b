#include <Python.h>

#include "lazy.h"

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Memoizer;

PyDoc_STRVAR(__doc__,
"TODO Memoizer __doc__");

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

/* Methods */

PyDoc_STRVAR(reap_doc,
"TODO Memoizer.reap __doc__");

static int
__contains__(PyObject *self, PyObject *key)
{
    return contains((Cache *)self, key);
}

static PyObject *
__getitem__(PyObject *self, PyObject *key)
{
    return get((Cache *)self, key);
}

static Py_ssize_t
__len__(PyObject *self)
{
    return length((Cache *)self);
}

static int
__setitem__(PyObject *self, PyObject *key, PyObject *value)
{
    return set((Cache *)self, key, value);
}

/* Type */

static PyMappingMethods as_mapping = {
    (lenfunc)__len__,           /* mp_length */
    (binaryfunc)__getitem__,    /* mp_subscript */
    (objobjargproc)__setitem__, /* mp_ass_subscript */
};

static PySequenceMethods as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    (objobjproc)__contains__,   /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

static PyMethodDef methods[] = {
    {"reap", (PyCFunction)reap, METH_NOARGS, reap_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject MemoizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Memoizer",                /* tp_name */
    sizeof(Memoizer),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)__del__,       /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    &as_sequence,              /* tp_as_sequence */
    &as_mapping,               /* tp_as_mapping */
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
    methods,                   /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)__new__,          /* tp_new */
};
