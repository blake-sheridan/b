#include <Python.h>

#define INITIAL_SIZE 128
#define PERTURB_SHIFT 5

#define USABLE(size) ((((size) << 1) + 1) / 3)

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

#define Cache_HEAD                              \
    PyObject *function;                         \
    Py_ssize_t size;                            \
    Py_ssize_t usable;                          \
    Entry (*entries)[1];

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Cache;

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Memoizer;

typedef struct {
    PyObject_HEAD
    Cache_HEAD
} Property;

/* http://burtleburtle.net/bob/hash/integer.html */
static inline Py_hash_t
hash_int(void* x)
{
    Py_hash_t a = (Py_hash_t)x;
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

static PyObject *
new(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     function->ob_type->tp_name);
        return NULL;
    }

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Cache *this = (Cache *)self;

    Py_INCREF(function);

    this->function = function;
    this->entries = NULL;

    return self;
}

static void
dealloc(PyObject *self)
{
    Cache *this = (Cache *)self;

    Py_DECREF(this->function);

    if (this->entries != NULL) {
        register Py_ssize_t i;
        register Py_ssize_t len = this->size;

        Entry *ep0 = (Entry *)&this->entries[0];

        for (i = 0; i < len; i++) {
            Py_XDECREF(ep0[i].value);
        }
    }

    self->ob_type->tp_free(self);
}

static int
grow(Cache *this) {
    Py_ssize_t old_size = this->size;
    Py_ssize_t new_size = old_size * 2;

    Entry (*old_entries)[old_size] = this->entries;
    Entry (*new_entries)[new_size] = PyMem_Malloc(new_size * sizeof(Entry));
    if (new_entries == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    Entry *old_ep0 = (Entry *)&old_entries[0];
    Entry *new_ep0 = (Entry *)&new_entries[0];

    register size_t i, j;
    register size_t perturb;
    register size_t mask;
    register Entry *old_ep, *new_ep;

    for (j = 0; j < new_size; j++) {
        new_ep0[j].key = NULL;
        new_ep0[j].value = NULL;
    }

    int count = 0;

    mask = new_size - 1;

    PyObject *key;
    Py_hash_t hash;

    for (i = 0; i < old_size; i++) {
        old_ep = &old_ep0[i];

        key = old_ep->key;

        if (key != NULL) {
            count++;

            hash = hash_int(key);

            j = hash & mask;

            new_ep = &new_ep0[j];

            if (new_ep->key == NULL) {
                new_ep->key   = key;
                new_ep->value = old_ep->value;
            } else {
                for (perturb = (size_t)key; ; perturb >>= PERTURB_SHIFT) {
                    j = (j << 2) + j + perturb + 1;

                    new_ep = &new_ep0[j & mask];

                    if (new_ep->key == NULL) {
                        new_ep->key   = key;
                        new_ep->value = old_ep->value;
                        break;
                    }
                }
            }
        }
    }

    PyMem_FREE(old_entries);

    this->entries = new_entries;
    this->size = new_size;
    this->usable = USABLE(new_size) - count;

    return 0;
}

static PyObject *
reap(Cache *this) {
    Py_ssize_t count = 0;

    Entry *ep0 = (Entry *)&this->entries[0];
    Py_ssize_t i, n;
    Entry *curr;

    for (i = 0, n = this->size; i < n; i++) {
        curr = &ep0[i];

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

static PyObject *
get(Cache *this, PyObject *key)
{
    PyObject *value;

    register size_t i;
    register size_t perturb;
    register size_t mask;
    register Entry *ep;
    Entry *ep0; // register-ing this slows by 33%

    if (this->entries == NULL) {
        this->entries = PyMem_Malloc(sizeof(Entry) * INITIAL_SIZE);
        if (this->entries == NULL) {
            PyErr_NoMemory();
            return NULL;
        }

        this->size = INITIAL_SIZE;
        this->usable = USABLE(INITIAL_SIZE);

        ep0 = (Entry *)&this->entries[0];

        for (i = 0; i < INITIAL_SIZE; i++) {
            ep0[i].key = NULL;
            ep0[i].value = NULL;
        }
    } else {
        ep0 = (Entry *) &this->entries[0];
    }

    mask = this->size - 1;

    Py_hash_t hash = hash_int(key);

    i = hash & mask;

    ep = &ep0[i];

    if (ep->key == key) {
        value = ep->value;
        Py_INCREF(value);
        return value;
    }

    if (ep->key == NULL)
        goto missing;

    for (perturb = key; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        ep = &ep0[i & mask];

        if (ep->key == key) {
            value = ep->value;
            Py_INCREF(value);
            return value;
        }

        if (ep->key == NULL)
            goto missing;
    }

  missing:
    value = PyObject_CallFunctionObjArgs(this->function, key, NULL);
    if (value == NULL)
        return NULL;

    ep->key = key;
    ep->value = value;

    if (this->usable-- <= 0)
        if (grow(this) == -1)
            return NULL;

    Py_INCREF(value);

    return value;
}

static int
contains(Cache *this, PyObject *key)
{
    if (this->entries == NULL)
        return 0;

    register size_t i;
    register size_t perturb;
    register Entry *ep;
    register size_t mask;
    Entry *ep0 = (Entry *)&this->entries[0]; // register-ing this slows by 33%

    mask = this->size - 1;

    Py_hash_t hash = hash_int(key);

    i = hash & mask;

    ep = &ep0[i];

    if (ep->key == key)
        return 1;

    if (ep->key == NULL)
        return 0;

    for (perturb = key; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        ep = &ep0[i & mask];

        if (ep->key == key)
            return 1;

        if (ep->key == NULL)
            return 0;
    }

    assert(0);
    return -1;
}

static int
set(PyObject *self, PyObject *instance, PyObject *value)
{
    Cache *this = (Cache *)self;

    register size_t i;
    register size_t perturb;
    register size_t mask;
    Entry *ep0;
    register Entry *ep;

    PyObject *key = instance;

    Py_hash_t hash = hash_int(key);

    mask = this->size - 1;

    ep0 = (Entry *)&this->entries[0];

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
    if (value == NULL)
        return -1;

    Py_INCREF(value);

    ep->key = key;
    ep->value = value;

    if (this->usable-- <= 0)
        if (grow(this) == -1)
            return -1;

    return 0;

  found:
    Py_DECREF(ep->value);

    if (value == NULL) {
        ep->key = NULL;
        ep->value = NULL;

        this->usable++;
    } else {
        Py_INCREF(value);

        ep->value = value;
    }

    return 0;
}

static Py_ssize_t
length(Cache *this) {
    return USABLE(this->size) - this->usable;
}

PyDoc_STRVAR(reap_doc,
"TODO Memoizer.reap __doc__");

static PyMethodDef Memoizer_methods[] = {
    //{"__getitem__", (PyCFunction)Cache_get, METH_O|METH_COEXIST, Cache_get_doc},
    {"reap", (PyCFunction)reap, METH_NOARGS, reap_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMappingMethods Memoizer_as_mapping = {
    (lenfunc)length, /* mp_length */
    (binaryfunc)get, /* mp_subscript */
    (objobjargproc)set, /* mp_ass_subscript */
};

static PySequenceMethods Memoizer_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    (objobjproc)contains,       /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

PyDoc_STRVAR(Memoizer_doc,
"TODO Memoizer __doc__");

PyTypeObject MemoizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Memoizer",                /* tp_name */
    sizeof(Memoizer),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)dealloc,       /* tp_dealloc */
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
    Memoizer_doc,              /* tp_doc */
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
    (newfunc)new,              /* tp_new */
};

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
