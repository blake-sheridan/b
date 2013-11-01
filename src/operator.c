#include <Python.h>

PyDoc_STRVAR(ExceptionRaiser__doc__,
"An operator which raises a specific exception if called."
);

typedef struct {
    PyObject_HEAD
    PyObject *type;
    PyObject *value;
} ExceptionRaiser;

static PyObject *
ExceptionRaiser__new__(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *arg;

    if (!PyArg_ParseTuple(args, "O", &arg))
        return NULL;

    PyObject *type, *value;

    if (PyExceptionClass_Check(arg)) {
        type = arg;
        value = Py_None;
    } else if (PyExceptionInstance_Check(arg)) {
        type = (PyObject *)Py_TYPE(arg);
        value = arg;
    } else {
        PyErr_Format(PyExc_TypeError,
                     "expected exception type or value, got: %o",
                     arg);
        return NULL;
    }

    PyObject *self = cls->tp_alloc(cls, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(type);
    Py_INCREF(value);

    ((ExceptionRaiser *)self)->type = type;
    ((ExceptionRaiser *)self)->value = value;

    return self;
}

static void
ExceptionRaiser__del__(PyObject *self)
{
    Py_DECREF(((ExceptionRaiser *)self)->type);
    Py_DECREF(((ExceptionRaiser *)self)->value);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
ExceptionRaiser__call__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetObject(((ExceptionRaiser *)self)->type, ((ExceptionRaiser *)self)->value);
    return NULL;
}

static PyTypeObject ExceptionRaiser_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ExceptionRaiser",         /* tp_name */
    sizeof(ExceptionRaiser),   /* tp_basicsize */
    0,                         /* tp_itemsize */
    ExceptionRaiser__del__,    /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    ExceptionRaiser__call__,   /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    ExceptionRaiser__doc__,    /* tp_doc */
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
    ExceptionRaiser__new__,    /* tp_new */
};
PyDoc_STRVAR(Module__doc__,
"TODO module __doc__");

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "b._operator",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__operator(void)
{
    PyObject *module = PyModule_Create(&Module);
    if (module == NULL)
        return NULL;

    /* ExceptionRaiser */

    if (PyType_Ready(&ExceptionRaiser_type) < 0)
        return NULL;

    Py_INCREF(&ExceptionRaiser_type);

    PyModule_AddObject(module, ExceptionRaiser_type.tp_name, (PyObject *)&ExceptionRaiser_type);

    return module;
};
