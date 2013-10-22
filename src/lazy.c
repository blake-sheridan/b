#include <Python.h>

#include <Python.h>

#define INITIAL_SIZE 128

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

typedef struct {
    PyObject *function;
    Entry (*entries)[];
    Py_ssize_t size;
} Table;

typedef struct {
    PyObject_HEAD
    Table *table;
} Memoizer;

typedef struct {
    PyObject_HEAD
    Table *table;
} Property;

/* Table */

static Table *
Table_new(PyObject *args, PyObject **kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function)) {
        return NULL;
    }

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     function->ob_type->tp_name);
        return NULL;
    }

    Table *this = PyMem_Malloc(sizeof(Table));
    if (this == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(function);

    this->function = function;
    this->entries = NULL;
    this->size = 0;

    return this;
}

static void
Table_free(Table *this)
{
    Py_XDECREF(this->function);

    if (this->size != 0) {
        register Entry (*entries)[] = this->entries;
        register Entry *entry;
        register size_t i;


        for (i = 0; i < this->size; i++) {
            entry = &(*entries)[i];

            if (entry->key != NULL) {
                Py_XDECREF(entry->value);
            }
        }

        PyMem_Free(this->entries);
    }

    PyMem_Free(this);
}

static PyObject *
Table_getitem(Table *this, PyObject *key)
{
    register Entry (*entries)[] = this->entries;
    register Entry *entry;
    register size_t i;
    PyObject *value;

    if (entries == NULL) {
        entries = this->entries = PyMem_Malloc(INITIAL_SIZE * sizeof(Entry));

        if (entries == NULL) {
            PyErr_NoMemory();
            return NULL;
        }

        this->size = INITIAL_SIZE;

        for (i = 0; i < this->size; i++) {
            (*entries)[i].key = NULL;
        }
    }

    entry = &(*entries)[(size_t)key % (this->size)];

    if (entry->key == key) {
        value = entry->value;
    } else {
        if (entry->key != NULL) {
            PyErr_SetString(PyExc_NotImplementedError, "resize");
            return NULL;
        }

        value = PyObject_CallFunctionObjArgs(this->function, key, NULL);
        if (value == NULL) {
            return NULL;
        }

        entry->key = key;
        entry->value = value;
    }

    Py_INCREF(value);

    return value;
}

/* Memoizer */

static PyObject *
Memoizer_new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    Table *table = Table_new(args, kwargs);
    if (table == NULL) {
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL; // should dealloc
    }

    ((Memoizer*)self)->table = table;

    return self;
}

static void
Memoizer_dealloc(PyObject *self)
{
    Table_free(((Memoizer*)self)->table);
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
    return Table_getitem(((Memoizer*)self)->table, key);
}

static int
Memoizer_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
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

/* property */

static PyObject *
property_new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    Table *table = Table_new(args, kwargs);
    if (table == NULL) {
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL; // should dealloc
    }

    ((Property*)self)->table = table;

    return self;
}

static void
property_dealloc(PyObject *self)
{
    Table_free(((Property*)self)->table);
    self->ob_type->tp_free(self);
}

static PyObject *
property_get_doc(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->table->function, "__doc__");
}

static PyObject *
property_get_name(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->table->function, "__name__");
}

static PyObject *
property_get_qualname(PyObject *self)
{
    return PyObject_GetAttrString(((Property*)self)->table->function, "__qualname__");
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

    return Table_getitem(((Property*)self)->table, instance);
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

/* Module */

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
