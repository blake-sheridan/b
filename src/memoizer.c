#include <Python.h>

#include "funcobject.h"

typedef struct {
    PyObject_HEAD
    PyObject *function;
} Memoizer;

static PyObject *
Memoizer_new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    if (PyTuple_GET_SIZE(args) != 1) {
        PyErr_SetString(PyExc_TypeError, "'Memoizer' takes a single argument");
        return NULL;
    }

    PyObject *function = PyTuple_GET_ITEM(args, 0);
    if (!PyFunction_Check(function)) {
        PyErr_SetString(PyExc_TypeError, "expecting function");
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    ((Memoizer*)self)->function = function;
    Py_INCREF(function);

    return self;
}

static void
Memoizer_dealloc(PyObject *self)
{
    Memoizer *this = (Memoizer*)self;

    Py_XDECREF(this->function);
    self->ob_type->tp_free(self);
}

static Py_ssize_t
Memoizer_length(PyObject *self)
{
    PyErr_SetString(PyExc_NotImplementedError, "Memoizer.__len__");
    return -1;
}

static PyObject *
Memoizer_subscript(PyObject *self, PyObject *key)
{
    PyErr_SetString(PyExc_NotImplementedError, "Memoizer.__getitem__");
    return NULL;
}

static int
Memoizer_ass_subscript(PyObject *self, PyObject *v, PyObject *w)
{
    PyErr_SetString(PyExc_NotImplementedError, "Memoizer.__setitem__");
    return -1;
}

static PyMappingMethods Memoizer_as_mapping = {
    (lenfunc)Memoizer_length, /* mp_length */
    (binaryfunc)Memoizer_subscript, /* mp_subscript */
    (objobjargproc)Memoizer_ass_subscript, /* mp_ass_subscript */
};

PyDoc_STRVAR(Memoizer_doc,
"TODO Memoizer __doc__");

PyTypeObject MemoizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Memoizer",                /* tp_name */
    sizeof(Memoizer),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)Memoizer_dealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    &Memoizer_as_mapping,      /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    Memoizer_doc,              /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    (newfunc)Memoizer_new,     /* tp_new */
};
