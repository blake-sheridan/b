#include <Python.h>

#define INITIAL_SIZE 16
#define PERTURB_SHIFT 5

#define USABLE(size) ((((size) << 1) + 1) / 3)

typedef struct {
    PyObject *key;
    PyObject *value;
} Entry;

typedef struct {
    PyObject_HEAD
    Py_ssize_t size;
    Py_ssize_t usable;
    Entry (*entries)[1];
} IdentityDict;

/* Forward */

static int grow(IdentityDict *this);

static PyTypeObject IdentityDictKeys_type;
static PyTypeObject IdentityDictItems_type;
static PyTypeObject IdentityDictValues_type;

static PyTypeObject IdentityDictKeysIterator_type;
static PyTypeObject IdentityDictItemsIterator_type;
static PyTypeObject IdentityDictValuesIterator_type;

static PyObject * IdentityDictIterator_new(PyTypeObject *type, IdentityDict *dict);
static PyObject * IdentityDictView_new(PyTypeObject *type, IdentityDict *dict);

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

/* IdentityDict */

PyDoc_STRVAR(IdentityDict__doc__,
"TODO IdentityDict.__doc__");

static PyObject *
IdentityDict__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_ssize_t size = INITIAL_SIZE;
    Entry (*entries)[1];

    entries = PyMem_Malloc(size * sizeof(Entry));
    if (entries == NULL)
        return NULL;

    Entry *entry_0 = (Entry *)&entries[0];

    Py_ssize_t i;
    for (i = 0; i < size; i++) {
        entry_0[i].key = NULL;
        entry_0[i].value = NULL;
    }

    ((IdentityDict *)self)->entries = entries;
    ((IdentityDict *)self)->size = size;
    ((IdentityDict *)self)->usable = USABLE(size);

    return self;
}

static void
IdentityDict__del__(PyObject *self)
{
    IdentityDict *this = (IdentityDict *)self;

    register Py_ssize_t i;
    register Py_ssize_t size = this->size;

    Entry *entry_0 = (Entry *)&this->entries[0];

    for (i = 0; i < size; i++) {
        Py_XDECREF(entry_0[i].key);
        Py_XDECREF(entry_0[i].value);
    }

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
IdentityDict__getitem__(PyObject *self, PyObject *key)
{
    IdentityDict *this = (IdentityDict *)self;

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
    PyErr_SetString(PyExc_NotImplementedError, "__missing__");
    return NULL;
}

static int
IdentityDict__setitem__(PyObject *self, PyObject *key, PyObject *value)
{
    IdentityDict *this = (IdentityDict *)self;

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

    Py_INCREF(key);
    Py_INCREF(value);

    entry->key = key;
    entry->value = value;

    if (this->usable-- < 1)
        if (grow(this) == -1)
            return -1;

    return 0;

  found:
    if (value == NULL) {
        entry->key = NULL;
        entry->value = NULL;

        Py_DECREF(entry->key);
        Py_DECREF(entry->value);

        this->usable++;
    } else {
        if (entry->key != key) {
            Py_DECREF(entry->key);
            Py_INCREF(key);
            entry->key = key;
        }

        Py_INCREF(value);

        entry->value = value;
    }

    return 0;
}

static int
IdentityDict__contains__(PyObject *self, PyObject *key)
{
    IdentityDict *this = (IdentityDict *)self;

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
IdentityDict__len__(PyObject *self)
{
    IdentityDict *this = (IdentityDict *)self;

    return USABLE(this->size) - this->usable;
}

/* Forward */

static PyObject *
IdentityDict__iter__(PyObject *self)
{
    return IdentityDictIterator_new(
        &IdentityDictKeysIterator_type,
        (IdentityDict *)self
        );
}

static int
grow(IdentityDict *this)
{
    Py_ssize_t old_size = this->size;
    Py_ssize_t new_size = old_size * 2;

    Entry *old_entry_0;
    Entry *new_entry_0;

    Entry (*old_entries)[old_size] = this->entries;
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

    this->entries = new_entries;
    this->size = new_size;
    this->usable = new_usable;

    return 0;
}

/*[clinic]
module IdentityDict

IdentityDict.get

  key: object
  default: object = NULL
  /

self[key] if key in self, else default (which is None if not provided).
[clinic]*/

PyDoc_STRVAR(IdentityDict_get__doc__,
"self[key] if key in self, else default (which is None if not provided).\n"
"\n"
"IdentityDict.get(key, default=None)");

#define IDENTITYDICT_GET_METHODDEF    \
    {"get", (PyCFunction)IdentityDict_get, METH_VARARGS, IdentityDict_get__doc__},

static PyObject *
IdentityDict_get_impl(PyObject *self, PyObject *key, PyObject *default_value);

static PyObject *
IdentityDict_get(PyObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = NULL;

    if (!PyArg_ParseTuple(args,
        "O|O:get",
        &key, &default_value))
        goto exit;
    return_value = IdentityDict_get_impl(self, key, default_value);

exit:
    return return_value;
}

static PyObject *
IdentityDict_get_impl(PyObject *self, PyObject *key, PyObject *default_value)
/*[clinic checksum: eeb64b68bb6f130ac11a4cbf38dad4932b3be342]*/
{
    PyObject *value;

    IdentityDict *this = (IdentityDict *)self;

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

    if (entry->key == key) {
        value = entry->value;
        Py_INCREF(value);
        return value;
    }

    if (entry->key == NULL) {
        value = default_value == NULL ? Py_None : default_value;
        Py_INCREF(value);
        return value;
    }

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;

        entry = &entry_0[i & mask];

        if (entry->key == key) {
            value = entry->value;
            Py_INCREF(value);
            return value;
        }

        if (entry->key == NULL) {
            value = default_value == NULL ? Py_None : default_value;
            Py_INCREF(value);
            return value;
        }
    }

    assert(0);
    return NULL;
}

/*[clinic]
module IdentityDict

IdentityDict.setdefault

  key: object
  default: object = NULL
  /

Return `self.get(key, default)` and assign `key` to `default` if absent.
[clinic]*/

PyDoc_STRVAR(IdentityDict_setdefault__doc__,
"Return `self.get(key, default)` and assign `key` to `default` if absent.\n"
"\n"
"IdentityDict.setdefault(key, default=None)");

#define IDENTITYDICT_SETDEFAULT_METHODDEF    \
    {"setdefault", (PyCFunction)IdentityDict_setdefault, METH_VARARGS, IdentityDict_setdefault__doc__},

static PyObject *
IdentityDict_setdefault_impl(PyObject *self, PyObject *key, PyObject *default_value);

static PyObject *
IdentityDict_setdefault(PyObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = NULL;

    if (!PyArg_ParseTuple(args,
        "O|O:setdefault",
        &key, &default_value))
        goto exit;
    return_value = IdentityDict_setdefault_impl(self, key, default_value);

exit:
    return return_value;
}

static PyObject *
IdentityDict_setdefault_impl(PyObject *self, PyObject *key, PyObject *default_value)
/*[clinic checksum: 18f699c16ff10235b7b6fc860d29fd73ee7029df]*/
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDict.setdefault");
    return NULL;
}

/*[clinic]
module IdentityDict

IdentityDict.pop

  key: object
  default: object = NULL
  /

Remove a specified key and return its corresponding value.

If absent and `default` is provided, return `default`.
If absent and `default` is NOT provided, raise `KeyError`.
[clinic]*/

PyDoc_STRVAR(IdentityDict_pop__doc__,
"Remove a specified key and return its corresponding value.\n"
"\n"
"IdentityDict.pop(key, default=None)\n"
"\n"
"If absent and `default` is provided, return `default`.\n"
"If absent and `default` is NOT provided, raise `KeyError`.");

#define IDENTITYDICT_POP_METHODDEF    \
    {"pop", (PyCFunction)IdentityDict_pop, METH_VARARGS, IdentityDict_pop__doc__},

static PyObject *
IdentityDict_pop_impl(PyObject *self, PyObject *key, PyObject *default_value);

static PyObject *
IdentityDict_pop(PyObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = NULL;

    if (!PyArg_ParseTuple(args,
        "O|O:pop",
        &key, &default_value))
        goto exit;
    return_value = IdentityDict_pop_impl(self, key, default_value);

exit:
    return return_value;
}

static PyObject *
IdentityDict_pop_impl(PyObject *self, PyObject *key, PyObject *default_value)
/*[clinic checksum: 32e484b32203c8ffdaed3eee54d29c8889d30432]*/
{
    IdentityDict *this = (IdentityDict *)self;

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

  found:
    value = entry->value;

    Py_DECREF(key);

    entry->key = NULL;
    entry->value = NULL;

    this->usable++;

    return value;

  missing:
    if (default_value == NULL) {
        /* Implementation detail, probably? */
        _PyErr_SetKeyError(key);
        return NULL;
    } else {
        Py_INCREF(default_value);
        return default_value;
    }
}

/*[clinic]
module IdentityDict

IdentityDict.popitem

Remove an return some (key, value) pair as a 2-tuple.

Raise `KeyError` if empty.
[clinic]*/

PyDoc_STRVAR(IdentityDict_popitem__doc__,
"Remove an return some (key, value) pair as a 2-tuple.\n"
"\n"
"IdentityDict.popitem()\n"
"\n"
"Raise `KeyError` if empty.");

#define IDENTITYDICT_POPITEM_METHODDEF    \
    {"popitem", (PyCFunction)IdentityDict_popitem, METH_NOARGS, IdentityDict_popitem__doc__},

static PyObject *
IdentityDict_popitem(PyObject *self)
/*[clinic checksum: 6e63caf3587426293b3a58cb4f31f0f6993ea30d]*/
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDict.popitem");
    return NULL;
}

/*[clinic]
module IdentityDict

IdentityDict.keys

Return a set-like object providing a view on D's keys.
[clinic]*/

PyDoc_STRVAR(IdentityDict_keys__doc__,
"Return a set-like object providing a view on D\'s keys.\n"
"\n"
"IdentityDict.keys()");

#define IDENTITYDICT_KEYS_METHODDEF    \
    {"keys", (PyCFunction)IdentityDict_keys, METH_NOARGS, IdentityDict_keys__doc__},

static PyObject *
IdentityDict_keys(PyObject *self)
/*[clinic checksum: 26bf04aae2bb683dd2ed3cfb892713c9242d5703]*/
{
    return IdentityDictView_new(
        &IdentityDictKeys_type,
        (IdentityDict *)self
        );
}

/*[clinic]
module IdentityDict

IdentityDict.items

Return an object providing a view on D's items.
[clinic]*/

PyDoc_STRVAR(IdentityDict_items__doc__,
"Return an object providing a view on D\'s items.\n"
"\n"
"IdentityDict.items()");

#define IDENTITYDICT_ITEMS_METHODDEF    \
    {"items", (PyCFunction)IdentityDict_items, METH_NOARGS, IdentityDict_items__doc__},

static PyObject *
IdentityDict_items(PyObject *self)
/*[clinic checksum: ab64d075b37f5e6df7ab75d21a13e8a59c23c7b7]*/
{
    return IdentityDictView_new(
        &IdentityDictItems_type,
        (IdentityDict *)self
        );
}

/*[clinic]
module IdentityDict

IdentityDict.values

Return a set-like object providing a view on D's values.
[clinic]*/

PyDoc_STRVAR(IdentityDict_values__doc__,
"Return a set-like object providing a view on D\'s values.\n"
"\n"
"IdentityDict.values()");

#define IDENTITYDICT_VALUES_METHODDEF    \
    {"values", (PyCFunction)IdentityDict_values, METH_NOARGS, IdentityDict_values__doc__},

static PyObject *
IdentityDict_values(PyObject *self)
/*[clinic checksum: 53333f1117776f2e6abb726f5161836de82d2f96]*/
{
    return IdentityDictView_new(
        &IdentityDictValues_type,
        (IdentityDict *)self
        );
}

/* Manually for now, as this one's too out there for argument clinic. */
#define IDENTITYDICT_UPDATE_METHODDEF    \
    {"update", (PyCFunction)IdentityDict_update, METH_VARARGS|METH_KEYWORDS, "TODO"},

static PyObject *
IdentityDict_update(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDict.update");
    return NULL;
}

/*[clinic]
module IdentityDict

IdentityDict.clear

Remove all items from D.
[clinic]*/

PyDoc_STRVAR(IdentityDict_clear__doc__,
"Remove all items from D.\n"
"\n"
"IdentityDict.clear()");

#define IDENTITYDICT_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)IdentityDict_clear, METH_NOARGS, IdentityDict_clear__doc__},

static PyObject *
IdentityDict_clear(PyObject *self)
/*[clinic checksum: 735c297484b6301ab6db91e67b195cd122d817c8]*/
{
    IdentityDict *this = (IdentityDict *)self;
    Py_ssize_t i, size;
    Entry *entry_0 = (Entry *)&this->entries[0];
    PyObject *key;

    for (i = 0, size = this->size; i < size; i++) {
        key = entry_0[i].key;

        if (key != NULL) {
            Py_DECREF(key);
            Py_DECREF(entry_0[i].value);
        }
    }

    this->usable = USABLE(this->size);

    Py_RETURN_NONE;
}

/*[clinic]
module IdentityDict

IdentityDict.copy

Return a shallow copy of D.
[clinic]*/

PyDoc_STRVAR(IdentityDict_copy__doc__,
"Return a shallow copy of D.\n"
"\n"
"IdentityDict.copy()");

#define IDENTITYDICT_COPY_METHODDEF    \
    {"copy", (PyCFunction)IdentityDict_copy, METH_NOARGS, IdentityDict_copy__doc__},

static PyObject *
IdentityDict_copy(PyObject *self)
/*[clinic checksum: 9dd173fedc5b12bd096378f66b7b9c4127e98150]*/
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDict.copy");
    return NULL;
}

static PyMethodDef
IdentityDict_methods[] = {
    IDENTITYDICT_GET_METHODDEF
    IDENTITYDICT_SETDEFAULT_METHODDEF
    IDENTITYDICT_POP_METHODDEF
    IDENTITYDICT_POPITEM_METHODDEF
    IDENTITYDICT_KEYS_METHODDEF
    IDENTITYDICT_ITEMS_METHODDEF
    IDENTITYDICT_VALUES_METHODDEF
    IDENTITYDICT_UPDATE_METHODDEF
    /* fromkeys? */
    IDENTITYDICT_CLEAR_METHODDEF
    IDENTITYDICT_COPY_METHODDEF
    {NULL, NULL} /* sentinel */
};

static PyMappingMethods
IdentityDict_as_mapping = {
    IdentityDict__len__,      /* mp_length */
    IdentityDict__getitem__,  /* mp_subscript */
    IdentityDict__setitem__,  /* mp_ass_subscript */
};

static PySequenceMethods
IdentityDict_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    IdentityDict__contains__,   /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

static PyTypeObject
IdentityDict_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDict",            /* tp_name */
    sizeof(IdentityDict),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    IdentityDict__del__,       /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    &IdentityDict_as_sequence, /* tp_as_sequence */
    &IdentityDict_as_mapping,  /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    IdentityDict__doc__,       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    IdentityDict__iter__,      /* tp_iter */
    0,                         /* tp_iternext */
    IdentityDict_methods,      /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    IdentityDict__new__,       /* tp_new */
};

/* IdentityDictIterator */

typedef struct {
    PyObject_HEAD
    IdentityDict *dict;
    Py_ssize_t index;
} IdentityDictIterator;

static PyObject *
IdentityDictIterator_new(PyTypeObject *type, IdentityDict *dict)
{
    IdentityDictIterator *this = PyObject_New(IdentityDictIterator, type);
    if (this == NULL)
        return NULL;

    Py_INCREF(dict);

    this->dict = dict;
    this->index = 0;

    return (PyObject *)this;
}

static void
IdentityDictIterator__del__(PyObject *self)
{
    Py_XDECREF(((IdentityDictIterator *)self)->dict);
    PyObject_Del(self);
}

static PyObject *
IdentityDictIterator__length_hint__(PyObject *self, PyObject *args)
{
    IdentityDictIterator *this = (IdentityDictIterator *)self;
    IdentityDict *dict = this->dict;
    Py_ssize_t length;

    if (dict == NULL) {
        length = 0;
    } else {
        /* TODO: account for count already iterated */
        length = IdentityDict__len__((PyObject *)dict);
    }

    return PyLong_FromSsize_t(length);
}

/* Iterator __next__ */

static PyObject *
IdentityDictKeysIterator__next__(PyObject *self)
{
    IdentityDictIterator *this = (IdentityDictIterator *)self;
    IdentityDict *dict = this->dict;

    if (dict == NULL)
        return NULL;

    Entry *entry_0 = (Entry *)&dict->entries[0];
    Py_ssize_t i, size;

    PyObject *key;

    for (i = this->index, size = dict->size; i < size; i++) {
        key = entry_0[i].key;

        if (key != NULL) {
            Py_INCREF(key);
            this->index = i + 1;
            return key;
        }
    }

    /* Exhausted */
    Py_DECREF(dict);
    this->dict = NULL;
    return NULL;
}

static PyObject *
IdentityDictItemsIterator__next__(PyObject *self)
{
    IdentityDictIterator *this = (IdentityDictIterator *)self;
    IdentityDict *dict = this->dict;

    if (dict == NULL)
        return NULL;

    Entry *entry_0 = (Entry *)&dict->entries[0];
    Py_ssize_t i, size;

    PyObject *key, *item, *value;

    for (i = this->index, size = dict->size; i < size; i++) {
        key = entry_0[i].key;

        if (key != NULL) {
            value = entry_0[i].value;

            item = PyTuple_New(2);
            if (item == NULL)
                return NULL;

            Py_INCREF(key);
            Py_INCREF(value);

            /* "steals" references */
            PyTuple_SET_ITEM(item, 0, key);
            PyTuple_SET_ITEM(item, 1, entry_0[i].value);

            this->index = i + 1;

            return item;
        }
    }

    /* Exhausted */
    Py_DECREF(dict);
    this->dict = NULL;
    return NULL;
}

static PyObject *
IdentityDictValuesIterator__next__(PyObject *self)
{
    IdentityDictIterator *this = (IdentityDictIterator *)self;
    IdentityDict *dict = this->dict;

    if (dict == NULL)
        return NULL;

    Entry *entry_0 = (Entry *)&dict->entries[0];
    Py_ssize_t i, size;

    PyObject *key, *value;

    for (i = this->index, size = dict->size; i < size; i++) {
        key = entry_0[i].key;

        if (key != NULL) {
            value = entry_0[i].value;

            Py_INCREF(value);

            this->index = i + 1;

            return value;
        }
    }

    /* Exhausted */
    Py_DECREF(dict);
    this->dict = NULL;
    return NULL;
}

static PyMethodDef
IdentityDictIterator_methods[] = {
    {"__length_hint__", IdentityDictIterator__length_hint__, METH_NOARGS, NULL},
    {NULL, NULL} /* sentinel */
};

static PyTypeObject
IdentityDictKeysIterator_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictKeysIterator",         /* tp_name */
    sizeof(IdentityDictIterator),       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    IdentityDictIterator__del__,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    IdentityDictKeysIterator__next__,   /* tp_iternext */
    IdentityDictIterator_methods,       /* tp_methods */
};

static PyTypeObject
IdentityDictItemsIterator_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictItemsIterator",        /* tp_name */
    sizeof(IdentityDictIterator),       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    IdentityDictIterator__del__,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    IdentityDictItemsIterator__next__,  /* tp_iternext */
    IdentityDictIterator_methods,       /* tp_methods */
};

static PyTypeObject
IdentityDictValuesIterator_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictValuesIterator",       /* tp_name */
    sizeof(IdentityDictIterator),       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    IdentityDictIterator__del__,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    IdentityDictValuesIterator__next__, /* tp_iternext */
    IdentityDictIterator_methods,       /* tp_methods */
};

/* Views */

typedef struct {
    PyObject_HEAD
    IdentityDict *dict;
} IdentityDictView;

static PyObject *
IdentityDictView_new(PyTypeObject *type, IdentityDict *dict)
{
    IdentityDictView *this = PyObject_New(IdentityDictView, type);
    if (this == NULL)
        return NULL;

    Py_INCREF(dict);

    this->dict = dict;

    return (PyObject *)this;
}

static void
IdentityDictView__del__(PyObject *self)
{
    Py_XDECREF(((IdentityDictView *)self)->dict);
    PyObject_Del(self);
}

static Py_ssize_t
IdentityDictView__len__(PyObject *self)
{
    return IdentityDict__len__((PyObject *)((IdentityDictView *)self)->dict);
}

/* View __contains__ */

static int
IdentityDictKeys__contains__(PyObject *self, PyObject *key)
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDictKeys.__contains__");
    return -1;
}

static int
IdentityDictItems__contains__(PyObject *self, PyObject *key)
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDictItems.__contains__");
    return -1;
}

static int
IdentityDictValues__contains__(PyObject *self, PyObject *key)
{
    PyErr_SetString(PyExc_NotImplementedError, "IdentityDictValues.__contains__");
    return -1;
}

/* View __iter__ */

static PyObject *
IdentityDictKeys__iter__(PyObject *self)
{
    return IdentityDictIterator_new(
        &IdentityDictKeysIterator_type,
        ((IdentityDictView *)self)->dict
        );
}


static PyObject *
IdentityDictItems__iter__(PyObject *self)
{
    return IdentityDictIterator_new(
        &IdentityDictItemsIterator_type,
        ((IdentityDictView *)self)->dict
        );
}


static PyObject *
IdentityDictValues__iter__(PyObject *self)
{
    return IdentityDictIterator_new(
        &IdentityDictValuesIterator_type,
        ((IdentityDictView *)self)->dict
        );
}

/* View as_sequence */

static PySequenceMethods
IdentityDictKeys_as_sequence = {
    IdentityDictView__len__,         /* sq_length */
    0,                               /* sq_concat */
    0,                               /* sq_repeat */
    0,                               /* sq_item */
    0,                               /* sq_slice */
    0,                               /* sq_ass_item */
    0,                               /* sq_ass_slice */
    IdentityDictKeys__contains__,    /* sq_contains */
};

static PySequenceMethods
IdentityDictItems_as_sequence = {
    IdentityDictView__len__,         /* sq_length */
    0,                               /* sq_concat */
    0,                               /* sq_repeat */
    0,                               /* sq_item */
    0,                               /* sq_slice */
    0,                               /* sq_ass_item */
    0,                               /* sq_ass_slice */
    IdentityDictItems__contains__,   /* sq_contains */
};

static PySequenceMethods
IdentityDictValues_as_sequence = {
    IdentityDictView__len__,         /* sq_length */
    0,                               /* sq_concat */
    0,                               /* sq_repeat */
    0,                               /* sq_item */
    0,                               /* sq_slice */
    0,                               /* sq_ass_item */
    0,                               /* sq_ass_slice */
    IdentityDictValues__contains__,  /* sq_contains */
};

/* View type */

static PyTypeObject
IdentityDictKeys_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictKeys",             /* tp_name */
    sizeof(IdentityDictView),       /* tp_basicsize */
    0,                              /* tp_itemsize */
    IdentityDictView__del__,        /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    &IdentityDictKeys_as_sequence,  /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    0,                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    IdentityDictKeys__iter__,       /* tp_iter */
};

static PyTypeObject
IdentityDictItems_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictItems",            /* tp_name */
    sizeof(IdentityDictView),       /* tp_basicsize */
    0,                              /* tp_itemsize */
    IdentityDictView__del__,        /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    &IdentityDictItems_as_sequence, /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    0,                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    IdentityDictItems__iter__,      /* tp_iter */
};

static PyTypeObject
IdentityDictValues_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IdentityDictValues",           /* tp_name */
    sizeof(IdentityDictView),       /* tp_basicsize */
    0,                              /* tp_itemsize */
    IdentityDictView__del__,        /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    &IdentityDictValues_as_sequence,/* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    0,                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    IdentityDictValues__iter__,     /* tp_iter */
};

/* NamedTupleField */

static const char FIELD_KEY[] = "__";

typedef struct {
    PyObject_HEAD
    PyObject *default_;
    PyObject *doc;
    Py_ssize_t index;
    PyObject *name;
    PyObject *owner_qualname;
    PyObject *qualname;
    PyObject *type;
} NamedTupleField;

typedef struct {
    PyHeapTypeObject heap_type;
    Py_ssize_t num_fields;
    PyObject *fields;
} NamedTupleMeta;

typedef struct {
    PyObject_VAR_HEAD
    PyObject *values[1];
} NamedTuple;

static PyTypeObject NamedTupleMeta_type;
static PyTypeObject NamedTuple_type;

static PyObject *
NamedTupleField__new__(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"type", "default", "doc", NULL};

    PyObject *type;
    PyObject *default_ = NULL;
    PyObject *doc = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|$OO!", kwlist,
                                     &PyType_Type, &type,
                                     &default_,
                                     &PyUnicode_Type, &doc))
        return NULL;

    PyObject *self = cls->tp_alloc(cls, 0);
    if (self == NULL)
        return NULL;

    NamedTupleField *this = (NamedTupleField *)self;

    Py_XINCREF(default_);
    this->default_ = default_;

    Py_XINCREF(doc);
    this->doc = doc;

    /* Set by Namespace */
    this->index = 0;
    this->name = NULL;

    Py_INCREF(type);
    this->type = type;

    return self;
}

static void
NamedTupleField__del__(PyObject *self)
{
    NamedTupleField *this = (NamedTupleField *)self;

    Py_XDECREF(this->default_);
    Py_XDECREF(this->name);
    Py_XDECREF(this->owner_qualname);
    Py_XDECREF(this->qualname);

    Py_DECREF(this->type);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
NamedTupleField__doc__(PyObject *self, void *_)
{
    PyObject *retval = ((NamedTupleField *)self)->doc;

    if (retval == NULL)
        retval = Py_None;

    Py_INCREF(retval);

    return retval;
}

static PyObject *
NamedTupleField__name__(PyObject *self, void *_)
{
    PyObject *retval = ((NamedTupleField *)self)->name;

    if (retval == NULL) {
        /* If accessed before even setting into classdef locals... */
        PyErr_SetString(PyExc_AttributeError, "__name__");
        return NULL;
    }

    Py_INCREF(retval);

    return retval;
}

static PyObject *
NamedTupleField__qualname__(PyObject *self, void *_)
{
    NamedTupleField *this = (NamedTupleField *)self;
    PyObject *qualname = this->qualname;

    if (qualname == NULL) {
        PyObject *owner_qualname = this->owner_qualname;

        if (owner_qualname == NULL) {
            /* If accessed inside classdef... */
            PyErr_SetString(PyExc_AttributeError, "__qualname__: owner never assigned");
            return NULL;
        }

        /* Don't think this can fail? */
        qualname = this->qualname = PyUnicode_FromFormat("%S.%S", owner_qualname, this->name);
    }

    Py_INCREF(qualname);

    return qualname;
}

static PyObject *
NamedTupleField__get__(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == NULL || instance == Py_None) {
        Py_INCREF(self);
        return self;
    }

/*
    if (!PyObject_TypeCheck(instance, &NamedTupleMeta_type)) {
        PyErr_Format(PyExc_TypeError, "Expecting NamedTuple, got: %R",
                     instance);
        return NULL;
    }
*/
    Py_ssize_t index = ((NamedTupleField *)self)->index;

    PyObject *value = ((NamedTuple *)instance)->values[index];

    Py_INCREF(value);

    return value;
}

static int
NamedTupleField__set__(PyObject *self, PyObject *instance, PyObject *value)
{
    PyErr_SetString(PyExc_AttributeError, "NamedTuples are immutable");
    return -1;
}

static PyGetSetDef
NamedTupleField_getset[] = {
    {"__doc__",      NamedTupleField__doc__,      NULL, "TODO"},
    {"__name__",     NamedTupleField__name__,     NULL, "TODO"},
    {"__qualname__", NamedTupleField__qualname__, NULL, "TODO"},
    {0}
};

static PyTypeObject
NamedTupleField_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.NamedTupleField",      /* tp_name */
    sizeof(NamedTupleField),        /* tp_basicsize */
    0,                              /* tp_itemsize */
    NamedTupleField__del__,         /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    0,                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    NamedTupleField_getset,         /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    NamedTupleField__get__,         /* tp_descr_get */
    NamedTupleField__set__,         /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    NamedTupleField__new__,         /* tp_new */
};

/* NamedTupleNamespace */

typedef struct {
    PyDictObject dict;
    PyObject *fields_list;
} NamedTupleNamespace;

static void
NamedTupleNamespace__del__(PyObject *self)
{
    Py_XDECREF(((NamedTupleNamespace *)self)->fields_list);
    PyDict_Type.tp_dealloc(self);
}

static int
NamedTupleNamespace__setitem__(PyObject *self, PyObject *key, PyObject *value)
{
    if (Py_TYPE(value) == &NamedTupleField_type) {
        PyObject *existing_value = PyDict_GetItem(self, key);
        if (existing_value != NULL) {
            PyErr_Format(PyExc_TypeError, "%R already assigned to %R",
                         key,
                         existing_value);
            return -1;
        }

        NamedTupleField *field = (NamedTupleField *)value;

        if (field->name != NULL) {
            PyErr_Format(PyExc_TypeError, "%R already assigned as %R",
                         field,
                         field->name);
            return -1;
        }

        Py_INCREF(key);
        field->name = key;

        if (PyList_Append(((NamedTupleNamespace *)self)->fields_list, value) == -1)
            return -1;
    }

    return PyDict_SetItem(self, key, value);
}

static int
NamedTupleNamespace_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
    if (value == NULL)
        return PyDict_DelItem(self, key);
    else
        return NamedTupleNamespace__setitem__(self, key, value);
}

static PyMappingMethods
NamedTupleNamespace_as_mapping = {
    0,                                   /* mp_length */
    0,                                   /* mp_subscript */
    NamedTupleNamespace_ass_subscript,   /* mp_ass_subscript */
};

static PyTypeObject
NamedTupleNamespace_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.NamedTupleNamespace",       /* tp_name */
    sizeof(NamedTupleNamespace),         /* tp_basicsize */
    0,                                   /* tp_itemsize */
    NamedTupleNamespace__del__,          /* tp_dealloc */
    0,                                   /* tp_print */
    0,                                   /* tp_getattr */
    0,                                   /* tp_setattr */
    0,                                   /* tp_reserved */
    0,                                   /* tp_repr */
    0,                                   /* tp_as_number */
    0,                                   /* tp_as_sequence */
    &NamedTupleNamespace_as_mapping,     /* tp_as_mapping */
    0,                                   /* tp_hash  */
    0,                                   /* tp_call */
    0,                                   /* tp_str */
    PyObject_GenericGetAttr,             /* tp_getattro */
    0,                                   /* tp_setattro */
    0,                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                  /* tp_flags */
    0,                                   /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    0,                                   /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    &PyDict_Type,                        /* tp_base */
};

/* NamedTupleMeta */

static PyObject *
NamedTupleMeta__prepare__(PyObject *self, PyObject *args)
{
    PyObject *namespace = NULL;
    PyObject *slots = NULL;

    if (PyType_Ready(&NamedTupleField_type) == -1)
        goto bail;

    if (PyType_Ready(&NamedTupleNamespace_type) == -1)
        goto bail;

    namespace = PyDict_Type.tp_new(&NamedTupleNamespace_type, NULL, NULL);
    if (namespace == NULL)
        goto bail;

    ((NamedTupleNamespace *)namespace)->fields_list = PyList_New(0);
    if (((NamedTupleNamespace *)namespace)->fields_list == NULL)
        goto bail;

    if (PyDict_SetItemString(namespace, FIELD_KEY, (PyObject *)&NamedTupleField_type) == -1)
        goto bail;

    slots = PyTuple_New(0);
    if (slots == NULL)
        goto bail;

    if (PyDict_SetItemString(namespace, "__slots__", slots) == -1)
        goto bail;

    Py_DECREF(slots);
    return namespace;

  bail:
    Py_XDECREF(namespace);
    Py_XDECREF(slots);
    return NULL;
}

static PyObject *
NamedTupleMeta__new__(PyTypeObject *mcs, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) != 3) {
        PyErr_SetString(PyExc_TypeError, "NamedTupleMeta: expecting 3 arguments");
        return NULL;
    }

    PyObject *namespace = PyTuple_GET_ITEM(args, 2);

    /* Sanity */

    if (Py_TYPE(namespace) != &NamedTupleNamespace_type) {
        PyErr_SetString(PyExc_TypeError, "NamedTupleMeta: expecting custom namespace");
        return NULL;
    }

    if (PyDict_DelItemString(namespace, FIELD_KEY) == -1)
        return NULL;

    /* Now let's see what we got... */

    PyObject *fields = PyList_AsTuple(((NamedTupleNamespace *)namespace)->fields_list);
    if (fields == NULL)
        return NULL;

    Py_ssize_t num_fields = PyTuple_GET_SIZE(fields);
    if (num_fields == 0) {
        PyErr_SetString(PyExc_TypeError, "NamedTupleMeta: no fields declared");
        return NULL;
    }

    PyObject *cls = PyType_Type.tp_new(mcs, args, kwargs);
    if (cls == NULL)
        return NULL;

    PyObject *owner_qualname = ((PyHeapTypeObject *)cls)->ht_qualname;
    NamedTupleField *field;
    Py_ssize_t i;

    for (i = 0; i < num_fields; i++) {
        field = (NamedTupleField *)PyTuple_GET_ITEM(fields, i);

        Py_XINCREF(owner_qualname);
        field->index = i;
        field->owner_qualname = owner_qualname;
    }

    ((NamedTupleMeta *)cls)->fields = fields;
    ((NamedTupleMeta *)cls)->num_fields = num_fields;

    return cls;
}

static Py_ssize_t
NamedTupleMeta__len__(PyObject *cls)
{
    return ((NamedTupleMeta *)cls)->num_fields;
}

static PyObject *
NamedTupleMeta__getitem__(PyObject *cls, Py_ssize_t index)
{
    NamedTupleMeta *this = (NamedTupleMeta *)cls;

    if (index < 0 || index >= this->num_fields) {
        PyErr_SetString(PyExc_IndexError, "NamedTupleMeta index out of range");
        return NULL;
    }

    PyObject *field = PyTuple_GET_ITEM(this->fields, index);
    Py_INCREF(field);
    return field;
}

static PyObject *
NamedTupleMeta__iter__(PyObject *cls)
{
    return PyObject_GetIter(((NamedTupleMeta *)cls)->fields);
}

static PySequenceMethods
NamedTupleMeta_as_sequence = {
    NamedTupleMeta__len__,         /* sq_length */
    0,                             /* sq_concat */
    0,                             /* sq_repeat */
    NamedTupleMeta__getitem__,     /* sq_item */
    0,                             /* sq_slice */
    0,                             /* sq_ass_item */
    0,                             /* sq_ass_slice */
    0,                             /* sq_contains */
};

static PyMethodDef
NamedTupleMeta_methods[] = {
    {"__prepare__", NamedTupleMeta__prepare__, METH_VARARGS | METH_CLASS, "TODO"},
    {NULL, NULL} /* sentinel */
};

static PyTypeObject
NamedTupleMeta_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.NamedTupleMeta",       /* tp_name */
    sizeof(NamedTupleMeta),         /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    &NamedTupleMeta_as_sequence,    /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    0,                              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    NamedTupleMeta__iter__,         /* tp_iter */
    0,                              /* tp_iternext */
    NamedTupleMeta_methods,         /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    &PyType_Type,                   /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    NamedTupleMeta__new__,          /* tp_new */
};

/* NamedTuple */

PyDoc_STRVAR(NamedTuple__doc__,
"A subclassing API approach to named tuples.");

static PyObject *
NamedTuple__new__(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    assert(PyType_IsSubtype(cls, &NamedTupleMeta_type));

    NamedTupleMeta *cls_this = (NamedTupleMeta *)cls;

    Py_ssize_t num_fields = cls_this->num_fields;
    Py_ssize_t num_args = PyTuple_GET_SIZE(args);

    if (kwargs != NULL) {
        PyErr_SetString(PyExc_NotImplementedError, "NamedTuple.__new__, **kwargs");
        return NULL;
    }

    PyObject *self = cls->tp_alloc(cls, num_fields);
    if (self == NULL)
        return NULL;

    NamedTuple *this = (NamedTuple *)self;

    PyObject *value;
    Py_ssize_t i;
    for (i = 0; i < num_fields; i++) {
        if (i < num_args) {
            value = PyTuple_GET_ITEM(args, i);

            /* TODO: type check */

            Py_INCREF(value);
            this->values[i] = value;
        } else {
            PyErr_SetString(PyExc_NotImplementedError, "NamedTuple.__new__, arg defaults");
            return NULL;
        }
    }

    return self;
}

static PyObject *
NamedTuple__repr__(PyObject *self)
{
    PyObject *retval = NULL;

    PyObject *buffer = PyUnicode_New(512, 255);
    if (buffer == NULL)
        return NULL;

    PyTypeObject *cls = Py_TYPE(self);

    PyObject *s = ((PyHeapTypeObject *)cls)->ht_qualname;
    Py_ssize_t s_length = PyUnicode_GET_LENGTH(s);

    if (PyUnicode_CopyCharacters(buffer, 0, s, 0, s_length) == -1)
        goto bail;

    Py_ssize_t i = s_length;

    if (PyUnicode_WriteChar(buffer, i++, '(') == -1)
        goto bail;

    PyObject *fields      = ((NamedTupleMeta *)cls)->fields;
    Py_ssize_t num_fields = ((NamedTupleMeta *)cls)->num_fields;

    int first = 1;

    NamedTupleField *field;
    PyObject *value;

    int field_index;
    for (field_index = 0; field_index < num_fields; field_index++) {
        field = (NamedTupleField *)PyTuple_GET_ITEM(fields, field_index);
        value = PyTuple_GET_ITEM(self, field_index);

        if (value == field->default_)
            continue;

        if (first) {
            first = 0;
        } else {
            if (PyUnicode_WriteChar(buffer, i++, ',') == -1)
                goto bail;
            if (PyUnicode_WriteChar(buffer, i++, ' ') == -1)
                goto bail;
        }

        s = field->name;
        s_length = PyUnicode_GET_LENGTH(s);

        if (PyUnicode_CopyCharacters(buffer, i, s, 0, s_length) == -1)
            goto bail;

        i += s_length;

        if (PyUnicode_WriteChar(buffer, i++, '=') == -1)
            goto bail;

        if (Py_EnterRecursiveCall(" while getting the repr of a NamedTuple"))
            goto bail;
        s = PyObject_Repr(value);
        Py_LeaveRecursiveCall();

        if (s == NULL)
            goto bail;

        s_length = PyUnicode_GET_LENGTH(s);

        if (PyUnicode_CopyCharacters(buffer, i, s, 0, s_length) == -1) {
            Py_DECREF(s);
            goto bail;
        } else {
            Py_DECREF(s);
        }

        i += s_length;
    }

    if (PyUnicode_WriteChar(buffer, i++, ')') == -1)
        goto bail;

    retval = PyUnicode_Substring(buffer, 0, i);
  bail:
    Py_DECREF(buffer);
    return retval;
}

static PyTypeObject
NamedTuple_type = {
    PyVarObject_HEAD_INIT(&NamedTupleMeta_type, 0)
    "b.types.NamedTuple",           /* tp_name */
    0,                              /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    NamedTuple__repr__,             /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,        /* tp_flags */
    NamedTuple__doc__,              /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    &PyTuple_Type,                  /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    NamedTuple__new__,              /* tp_new */
};

/* Module */

PyDoc_STRVAR(Module__doc__,
"TODO collections __doc__");

static struct PyModuleDef
Module = {
    PyModuleDef_HEAD_INIT,
    "b._collections",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__collections(void)
{
    PyObject *module = PyModule_Create(&Module);

    if (module == NULL)
        return NULL;

    /* IdentityDict */

    if (PyType_Ready(&IdentityDict_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDict_type);

    PyModule_AddObject(module, "IdentityDict", (PyObject *)&IdentityDict_type);

    /* IdentityDictKeys */

    if (PyType_Ready(&IdentityDictKeys_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictKeys_type);

    PyModule_AddObject(module, "_IdentityDictKeys", (PyObject *)&IdentityDictKeys_type);

    /* IdentityDictItems */

    if (PyType_Ready(&IdentityDictItems_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictItems_type);

    PyModule_AddObject(module, "_IdentityDictItems", (PyObject *)&IdentityDictItems_type);

    /* IdentityDictValues */

    if (PyType_Ready(&IdentityDictValues_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictValues_type);

    PyModule_AddObject(module, "_IdentityDictValues", (PyObject *)&IdentityDictValues_type);

    /* IdentityDictKeysIterator */

    if (PyType_Ready(&IdentityDictKeysIterator_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictKeysIterator_type);

    PyModule_AddObject(module, "_IdentityDictKeysIterator", (PyObject *)&IdentityDictKeysIterator_type);

    /* IdentityDictItemsIterator */

    if (PyType_Ready(&IdentityDictItemsIterator_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictItemsIterator_type);

    PyModule_AddObject(module, "_IdentityDictItemsIterator", (PyObject *)&IdentityDictItemsIterator_type);

    /* IdentityDictValuesIterator */

    if (PyType_Ready(&IdentityDictValuesIterator_type) < 0)
        return NULL;

    Py_INCREF(&IdentityDictValuesIterator_type);

    PyModule_AddObject(module, "_IdentityDictValuesIterator", (PyObject *)&IdentityDictValuesIterator_type);

    /* NamedTupleMeta */

    if (PyType_Ready(&NamedTupleMeta_type) < 0)
        return NULL;

    Py_INCREF(&NamedTupleMeta_type);

    PyModule_AddObject(module, "_NamedTupleMeta", (PyObject *)&NamedTupleMeta_type);

    /* NamedTuple */

    if (PyType_Ready(&NamedTuple_type) < 0)
        return NULL;

    Py_INCREF(&NamedTuple_type);

    PyModule_AddObject(module, "NamedTuple", (PyObject *)&NamedTuple_type);

    return module;
};
