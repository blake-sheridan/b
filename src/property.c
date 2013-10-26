#include <Python.h>

#include "memoizer.h"

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Memoizer *memoizer;
} Property;

PyDoc_STRVAR(__doc__,
"TODO property __doc__");

static Property *
__new__(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    Property *self;
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     Py_TYPE(function)->tp_name);
        return NULL;
    }

    self = (Property *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(function);

    self->function = function;
    self->memoizer = NULL;

    return self;
}

static void
__del__(Property *self)
{
    Py_DECREF(self->function);
    Py_XDECREF(self->memoizer);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
__doc__getter(Property *self)
{
    return PyObject_GetAttrString(self->function, "__doc__");
}

static PyObject *
__name__getter(Property *self)
{
    return PyObject_GetAttrString(self->function, "__name__");
}

static PyObject *
__qualname__getter(Property *self)
{
    return PyObject_GetAttrString(self->function, "__qualname__");
}

static PyGetSetDef getset[] = {
    {"__doc__",      (getter)__doc__getter},
    {"__name__",     (getter)__name__getter},
    {"__qualname__", (getter)__qualname__getter},
    {0}
};

static PyObject *
__get__(Property *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return (PyObject *)self;
    }

    if (self->memoizer == NULL) {
        self->memoizer = Memoizer_new(self->function);
        if (self->memoizer == NULL)
            return NULL;
    }

    return Memoizer_get(self->memoizer, instance);
}

static int
__set__(Property *self, PyObject *instance, PyObject *value)
{
    if (self->memoizer == NULL) {
        self->memoizer = Memoizer_new(self->function);
        if (self->memoizer == NULL)
            return -1;
    }

    return Memoizer_assign(self->memoizer, instance, value);
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
