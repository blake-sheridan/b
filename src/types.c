#include "Python.h"

#include "hash.h"

/* LazyProperty */

typedef struct {
    PyObject_HEAD
    PyObject *function;
    PyObject *memoizer;
} LazyProperty;

/* Forward */
static PyObject *
Memoizer_new(PyObject *);

static PyObject *
Memoizer__getitem__(PyObject *, PyObject *);

static int
Memoizer_ass_subscript(PyObject *, PyObject *, PyObject *);

PyDoc_STRVAR(LazyProperty__doc__,
"TODO property __doc__");

static PyObject *
LazyProperty__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     Py_TYPE(function)->tp_name);
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(function);

    ((LazyProperty *)self)->function = function;
    ((LazyProperty *)self)->memoizer = NULL;

    return self;
}

static void
LazyProperty__del__(PyObject *self)
{
    Py_DECREF(((LazyProperty *)self)->function);
    Py_XDECREF(((LazyProperty *)self)->memoizer);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
LazyProperty__doc__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__doc__");
}

static PyObject *
LazyProperty__name__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__name__");
}

static PyObject *
LazyProperty__qualname__getter(PyObject *self, void *_)
{
    return PyObject_GetAttrString(((LazyProperty *)self)->function, "__qualname__");
}

static PyObject *
LazyProperty__get__(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == Py_None || instance == NULL) {
        Py_INCREF(self);
        return self;
    }

    LazyProperty *this = (LazyProperty *)self;

    if (this->memoizer == NULL) {
        this->memoizer = Memoizer_new(this->function);
        if (this->memoizer == NULL)
            return NULL;
    }

    return Memoizer__getitem__(this->memoizer, instance);
}

static int
LazyProperty__set__(PyObject *self, PyObject *instance, PyObject *value)
{
    LazyProperty *this = (LazyProperty *)self;

    if (this->memoizer == NULL) {
        this->memoizer = Memoizer_new(this->function);
        if (this->memoizer == NULL)
            return -1;
    }

    return Memoizer_ass_subscript(this->memoizer, instance, value);
}

static PyGetSetDef
LazyProperty_getset[] = {
    {"__doc__",      LazyProperty__doc__getter},
    {"__name__",     LazyProperty__name__getter},
    {"__qualname__", LazyProperty__qualname__getter},
    {0}
};

static PyTypeObject
LazyProperty_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b._types.lazyproperty",   /* tp_name */
    sizeof(LazyProperty),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    LazyProperty__del__,       /* tp_dealloc */
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
    LazyProperty__doc__,       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    LazyProperty_getset,       /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    LazyProperty__get__,       /* tp_descr_get */
    LazyProperty__set__,       /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    LazyProperty__new__,       /* tp_new */
};

/* Memoizer */

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Py_ssize_t size;
    Py_ssize_t usable;
    Entry (*entries)[1];
} Memoizer;

static PyTypeObject Memoizer_type;

PyDoc_STRVAR(Memoizer__doc__,
"TODO Memoizer __doc__");

static int
Memoizer_grow(Memoizer *self)
{
    Py_ssize_t old_size = self->size;
    Py_ssize_t new_size = old_size * 2;

    Entry *old_entry_0;
    Entry *new_entry_0;

    Entry (*old_entries)[old_size] = self->entries;
    Entry (*new_entries)[new_size] = PyMem_Malloc(new_size * sizeof(Entry));

    register size_t i, j;
    register size_t perturb;
    register size_t mask;
    register Entry *old_entry, *new_entry;

    Py_ssize_t new_usable = USABLE(new_size);

    PyObject *key;
    Py_hash_t hash;

    if (new_entries == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    old_entry_0 = (Entry *)&old_entries[0];
    new_entry_0 = (Entry *)&new_entries[0];

    for (j = 0; j < new_size; j++) {
        new_entry_0[j].key = NULL;
        new_entry_0[j].value = NULL;
    }

    mask = new_size - 1;

    for (i = 0; i < old_size; i++) {
        old_entry = &old_entry_0[i];

        key = old_entry->key;

        if (key != NULL) {
            new_usable--;

            hash = hash_int(key);

            j = hash & mask;

            new_entry = &new_entry_0[j];

            if (new_entry->key == NULL) {
                new_entry->key   = key;
                new_entry->value = old_entry->value;
            } else {
                for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
                    j = (j << 2) + j + perturb + 1;

                    new_entry = &new_entry_0[j & mask];

                    if (new_entry->key == NULL) {
                        new_entry->key   = key;
                        new_entry->value = old_entry->value;
                        break;
                    }
                }
            }
        }
    }

    PyMem_FREE(old_entries);

    self->entries = new_entries;
    self->size = new_size;
    self->usable = new_usable;

    return 0;
}
static int
Memoizer_insert(Memoizer *self, Entry *entry, PyObject *key, PyObject *value)
{
    Py_INCREF(value);

    entry->key = key;
    entry->value = value;

    if (self->usable-- > 0)
        return 0;
    else
        return Memoizer_grow(self);
}

static PyObject *
Memoizer_new(PyObject *function)
{
    PyObject *self;

#define INITIAL_SIZE 128
    Py_ssize_t size = INITIAL_SIZE;
    Py_ssize_t i;

    Entry (*entries)[1];
    Entry *entry_0;

    PyTypeObject *type = &Memoizer_type;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     Py_TYPE(function)->tp_name);
        return NULL;
    }

    self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    entries = PyMem_Malloc(size * sizeof(Entry));
    if (entries == NULL)
        return NULL;

    entry_0 = (Entry *)&entries[0];

    for (i = 0; i < size; i++) {
        entry_0[i].key = NULL;
        entry_0[i].value = NULL;
    }

    Py_INCREF(function);

    ((Memoizer *)self)->entries = entries;
    ((Memoizer *)self)->function = function;
    ((Memoizer *)self)->size = size;
    ((Memoizer *)self)->usable = USABLE(size);

    return self;
}

static PyObject *
Memoizer__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    return Memoizer_new(function);
}

static void
Memoizer__del__(PyObject *self)
{
    Memoizer *this = (Memoizer *)self;

    register Py_ssize_t i;
    register Py_ssize_t size = this->size;

    Entry *entry_0 = (Entry *)&this->entries[0];

    Py_DECREF(this->function);

    for (i = 0; i < size; i++) {
        Py_XDECREF(entry_0[i].value);
    }

    Py_TYPE(self)->tp_free(self);
}

PyDoc_STRVAR(Memoizer_reap__doc__,
"TODO Memoizer.reap __doc__");

static PyObject *
Memoizer_reap(PyObject *self, PyObject *_) {
    Memoizer *this = (Memoizer *)self;

    Py_ssize_t count = 0;

    Entry *entry_0 = (Entry *)&this->entries[0];
    Py_ssize_t i, n;
    Entry *curr;

    for (i = 0, n = this->size; i < n; i++) {
        curr = &entry_0[i];

        if (curr->key != NULL) {
            // FIXME: this is very much a giant hack,
            // basically checking if a heap pointer
            // 'appears' to have been freed/reused.
            if (Py_REFCNT(curr->key) > 100) {
                Py_DECREF(curr->value);

                curr->key = NULL;
                curr->value = NULL;

                this->usable++;

                count++;
            }
        }
    }

    return PyLong_FromSsize_t(count);
}

static int
Memoizer__contains__(PyObject *self, PyObject *key)
{
    Memoizer *this = (Memoizer *)self;

    register size_t i;
    register size_t perturb;
    register Entry *entry;
    register size_t mask;

    Entry *entry_0;
    Py_hash_t hash;

    entry_0 = (Entry *)&this->entries[0];

    mask = this->size - 1;

    hash = hash_int(key);

    i = hash & mask;

    entry = &entry_0[i];

    if (entry->key == key)
        return 1;

    if (entry->key == NULL)
        return 0;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        entry = &entry_0[i & mask];

        if (entry->key == key)
            return 1;

        if (entry->key == NULL)
            return 0;
    }

    assert(0);
    return -1;
}

static Py_ssize_t
Memoizer__len__(PyObject *self)
{
    return USABLE(((Memoizer *)self)->size) - ((Memoizer *)self)->usable;
}

static PyObject *
Memoizer__getitem__(PyObject *self, PyObject *key)
{
    Memoizer *this = (Memoizer *)self;

    PyObject *value;

    register size_t i;
    register size_t perturb;
    register size_t mask;
    register Entry *entry;
    Entry *entry_0; // register-ing this slows by 33%
    Py_hash_t hash;

    entry_0 = (Entry *)&this->entries[0];

    mask = this->size - 1;

    hash = hash_int(key);

    i = hash & mask;

    entry = &entry_0[i];

    if (entry->key == key) {
        value = entry->value;
        Py_INCREF(value);
        return value;
    }

    if (entry->key == NULL)
        goto missing;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        entry = &entry_0[i & mask];

        if (entry->key == key) {
            value = entry->value;
            Py_INCREF(value);
            return value;
        }

        if (entry->key == NULL)
            goto missing;
    }

  missing:
    value = PyObject_CallFunctionObjArgs(this->function, key, NULL);

    if (value == NULL)
        return NULL;

    if (Memoizer_insert(this, entry, key, value) == -1)
        return NULL;

    return value;
}

static int
Memoizer_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
    Memoizer *this = (Memoizer *)self;

    register size_t i;
    register size_t perturb;
    register size_t mask;
    Entry *entry_0;
    register Entry *entry;

    Py_hash_t hash = hash_int(key);

    mask = this->size - 1;

    entry_0 = (Entry *)&this->entries[0];

    i = (size_t)hash & mask;

    entry = &entry_0[i];

    if (entry->key == key)
        goto found;

    if (entry->key == NULL)
        goto missing;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        entry = &entry_0[i & mask];

        if (entry->key == key)
            goto found;

        if (entry->key == NULL)
            goto missing;
    }

  missing:
    if (value == NULL)
        return -1;

    return Memoizer_insert(this, entry, key, value);

  found:
    Py_DECREF(entry->value);

    if (value == NULL) {
        entry->key = NULL;
        entry->value = NULL;

        this->usable++;
    } else {
        Py_INCREF(value);

        entry->value = value;
    }

    return 0;
}

static PyMappingMethods
Memoizer_as_mapping = {
    Memoizer__len__,        /* mp_length */
    Memoizer__getitem__,    /* mp_subscript */
    Memoizer_ass_subscript, /* mp_ass_subscript */
};

static PySequenceMethods
Memoizer_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    Memoizer__contains__,       /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

static PyMethodDef
Memoizer_methods[] = {
    {"reap", Memoizer_reap, METH_NOARGS, Memoizer_reap__doc__},
    {NULL, NULL}           /* sentinel */
};

static PyTypeObject
Memoizer_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b._types.Memoizer",       /* tp_name */
    sizeof(Memoizer),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    Memoizer__del__,           /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    &Memoizer_as_sequence,     /* tp_as_sequence */
    &Memoizer_as_mapping,      /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    Memoizer__doc__,           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Memoizer_methods,          /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    Memoizer__new__,           /* tp_new */
};

/* Module */

PyDoc_STRVAR(Module__doc__,
"TODO module __doc__");

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "b._types",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__types(void)
{
    PyObject *module = PyModule_Create(&Module);

    if (module == NULL)
        return NULL;

    /* LazyProperty */

    if (PyType_Ready(&LazyProperty_type) < 0)
        return NULL;

    Py_INCREF(&LazyProperty_type);

    PyModule_AddObject(module, "lazyproperty", (PyObject *)&LazyProperty_type);

    /* Memoizer */

    if (PyType_Ready(&Memoizer_type) < 0)
        return NULL;

    Py_INCREF(&Memoizer_type);

    PyModule_AddObject(module, "Memoizer", (PyObject *)&Memoizer_type);

    return module;
};
