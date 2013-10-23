#include <Python.h>

#define INITIAL_SIZE 128
#define PERTURB_SHIFT 5

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

typedef struct {
    PyObject *function;
    Py_ssize_t size;
    Py_ssize_t usable;
    Py_ssize_t used;
    Entry entries[1];
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
    Py_ssize_t size = INITIAL_SIZE;

    if (!PyArg_ParseTuple(args, "O", &function)) {
        return NULL;
    }

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     function->ob_type->tp_name);
        return NULL;
    }

    Table *this = PyMem_Malloc(sizeof(Table) +
                               sizeof(Entry) * (size-1));
    if (this == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    Entry *ep0 = &this->entries[0];
    Py_ssize_t i;
    for (i = 0; i < size; i++) {
        ep0[i].key = NULL;
        ep0[i].value = NULL;
    }

    Py_INCREF(function);

    this->function = function;
    this->size = size;
    this->usable = (((size << 1)+1)/3);
    this->used = 0;

    return this;
}

static void
Table_free(Table *this)
{
    Py_XDECREF(this->function);

    Entry *entries = &this->entries[0];
    Py_ssize_t i, n;
    for (i = 0, n = this->size; i < n; i++) {
        Py_XDECREF(entries[i].key);
        Py_XDECREF(entries[i].value);
    }

    PyMem_FREE(this);
}

static int
Table_resize(Table *this) {
    PyErr_SetString(PyExc_NotImplementedError, "resize");
    return -1;

/*
    Py_ssize_t old_size = this->size;
    Py_ssize_t new_size = old_size * 2;

    Entry (*old_entries)[] = this->entries;
    Entry (*new_entries)[] = PyMem_Malloc(new_size * sizeof(Entry));
    if (new_entries == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    this->entries = new_entries;
    this->size = new_size;

    int i;

    for (i = 0; i < new_size; i++) {
        (*new_entries)[i].key = NULL;
    }

    PyObject *key;
    int j;
    Entry *entry;

    for (i = 0; i < old_size; i++) {
        key = (*old_entries)[i].key;

        if (key != NULL) {
            j = (size_t)key % new_size;

            while (1) {
                entry = &(*new_entries)[j];

                if (entry->key == NULL) {
                    entry->key = key;
                    entry->value = (*old_entries)[i].value;
                    break;
                }

                if (j == new_size) {
                    j = 0;
                } else {
                    j++;
                }
            }
        }
    }

    PyMem_Free(old_entries);

    return 0;
*/
}

static inline PyObject *
Table_getitem(Table *this, PyObject *key)
{
    register size_t i;
    register size_t perturb;
    register size_t mask;
    Entry *ep0;
    register Entry *ep;

    Py_hash_t hash = (Py_hash_t)key;

    mask = this->size - 1;

    ep0 = &this->entries[0];

    i = (size_t)hash & mask;

    ep = &ep0[i];

    if (ep->key == key)
        goto found;

    if (ep->key == NULL)
        goto missing;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        ep = &ep0[i & mask];

        if (ep->key == key)
            goto found;

        if (ep->key == NULL)
            goto missing;
    }

  missing:
    ep->value = PyObject_CallFunctionObjArgs(this->function, key, NULL);

    if (ep->value == NULL)
        return NULL;

    ep->key = key;

    this->used++;

    if (this->usable-- <= 0) {
        if (Table_resize(this) == -1)
            return NULL;
    }

  found:
    Py_INCREF(ep->value);
    return ep->value;
}

static int
Table_delitem(Table *this, PyObject *key)
{
    register size_t i;
    register size_t perturb;
    register size_t mask;
    Entry *ep0;
    register Entry *ep;

    Py_hash_t hash = (Py_hash_t)key;

    mask = this->size - 1;

    ep0 = &this->entries[0];

    i = (size_t)hash & mask;

    ep = &ep0[i];

    if (ep->key == key)
        goto found;

    if (ep->key == NULL)
        goto missing;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        ep = &ep0[i & mask];

        if (ep->key == key)
            goto found;

        if (ep->key == NULL)
            goto missing;
    }

  missing:
    return -1;
  found:
    Py_DECREF(ep->value);

    ep->key = NULL;
    ep->value = NULL;

    this->used--;

    return 0;
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
    return ((Memoizer*)self)->table->used;
}

static PyObject *
Memoizer_subscript(PyObject *self, PyObject *key)
{
    return Table_getitem(((Memoizer*)self)->table, key);
}

static int
Memoizer_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
    if (value == NULL) {
        return Table_delitem(((Memoizer*)self)->table, key);
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "Memoizer.__setitem__");
        return -1;
    }
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
    if (value == NULL) {
        return Table_delitem(((Property*)self)->table, instance);
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "__set__");
        return -1;
    }
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
