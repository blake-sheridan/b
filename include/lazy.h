#ifndef LAZY_H_
#define LAZY_H_

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
    PyObject *self;
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     function->ob_type->tp_name);
        return NULL;
    }

    self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(function);

    ((Cache *)self)->function = function;
    ((Cache *)self)->entries = NULL;

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

    Entry *old_ep0;
    Entry *new_ep0;

    Entry (*old_entries)[old_size] = this->entries;
    Entry (*new_entries)[new_size] = PyMem_Malloc(new_size * sizeof(Entry));

    register size_t i, j;
    register size_t perturb;
    register size_t mask;
    register Entry *old_ep, *new_ep;

    Py_ssize_t new_usable = USABLE(new_size);

    PyObject *key;
    Py_hash_t hash;

    if (new_entries == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    old_ep0 = (Entry *)&old_entries[0];
    new_ep0 = (Entry *)&new_entries[0];

    for (j = 0; j < new_size; j++) {
        new_ep0[j].key = NULL;
        new_ep0[j].value = NULL;
    }

    mask = new_size - 1;

    for (i = 0; i < old_size; i++) {
        old_ep = &old_ep0[i];

        key = old_ep->key;

        if (key != NULL) {
            new_usable--;

            hash = hash_int(key);

            j = hash & mask;

            new_ep = &new_ep0[j];

            if (new_ep->key == NULL) {
                new_ep->key   = key;
                new_ep->value = old_ep->value;
            } else {
                for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
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
    this->usable = new_usable;

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
    Py_hash_t hash;

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

    hash = hash_int(key);

    i = hash & mask;

    ep = &ep0[i];

    if (ep->key == key) {
        value = ep->value;
        Py_INCREF(value);
        return value;
    }

    if (ep->key == NULL)
        goto missing;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
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
    register size_t i;
    register size_t perturb;
    register Entry *ep;
    register size_t mask;

    Entry *ep0; // register-ing this slows by 33%
    Py_hash_t hash;

    if (this->entries == NULL)
        return 0;

    ep0 = (Entry *)&this->entries[0];

    mask = this->size - 1;

    hash = hash_int(key);

    i = hash & mask;

    ep = &ep0[i];

    if (ep->key == key)
        return 1;

    if (ep->key == NULL)
        return 0;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
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

#endif
