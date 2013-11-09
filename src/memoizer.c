#include <Python.h>

#include "memoizer.h"

#define INITIAL_SIZE 128
#define PERTURB_SHIFT 5

#define USABLE(size) ((((size) << 1) + 1) / 3)

PyTypeObject MemoizerType;

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

static int
grow(Memoizer *self)
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
insert(Memoizer *self, Entry *entry, PyObject *key, PyObject *value)
{
    Py_INCREF(value);

    entry->key = key;
    entry->value = value;

    if (self->usable-- > 0)
        return 0;
    else
        return grow(self);
}

PyDoc_STRVAR(__doc__,
"TODO Memoizer __doc__");

Memoizer *
Memoizer_new(PyObject *function)
{
    Memoizer *self;

    Py_ssize_t size = INITIAL_SIZE;
    Py_ssize_t i;

    Entry (*entries)[1];
    Entry *entry_0;

    PyTypeObject *type = &MemoizerType;

    if (!PyCallable_Check(function)) {
        PyErr_Format(PyExc_TypeError,
                     "expected callable, got: %s",
                     Py_TYPE(function)->tp_name);
        return NULL;
    }

    self = (Memoizer *)type->tp_alloc(type, 0);
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

    self->entries = entries;
    self->function = function;
    self->size = size;
    self->usable = USABLE(size);

    return self;
}

static Memoizer *
__new__(PyTypeObject *type, PyObject *args, PyObject **kwargs)
{
    PyObject *function;

    if (!PyArg_ParseTuple(args, "O", &function))
        return NULL;

    return Memoizer_new(function);
}

static void
__del__(Memoizer *self)
{
    register Py_ssize_t i;
    register Py_ssize_t size = self->size;

    Entry *entry_0 = (Entry *)&self->entries[0];

    Py_DECREF(self->function);

    for (i = 0; i < size; i++) {
        Py_XDECREF(entry_0[i].value);
    }

    Py_TYPE(self)->tp_free(self);
}

/* Methods */

PyDoc_STRVAR(reap_doc,
"TODO Memoizer.reap __doc__");

static PyObject *
reap(Memoizer *self) {
    Py_ssize_t count = 0;

    Entry *entry_0 = (Entry *)&self->entries[0];
    Py_ssize_t i, n;
    Entry *curr;

    for (i = 0, n = self->size; i < n; i++) {
        curr = &entry_0[i];

        if (curr->key != NULL) {
            // FIXME: this is very much a giant hack,
            // basically checking if a heap pointer
            // 'appears' to have been freed/reused.
            if (Py_REFCNT(curr->key) > 100) {
                Py_DECREF(curr->value);

                curr->key = NULL;
                curr->value = NULL;

                self->usable++;

                count++;
            }
        }
    }

    return PyLong_FromSsize_t(count);
}

static int
__contains__(Memoizer *self, PyObject *key)
{
    register size_t i;
    register size_t perturb;
    register Entry *entry;
    register size_t mask;

    Entry *entry_0;
    Py_hash_t hash;

    entry_0 = (Entry *)&self->entries[0];

    mask = self->size - 1;

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
__len__(Memoizer *self)
{
    return USABLE(self->size) - self->usable;
}

PyObject *
Memoizer_get(Memoizer *self, PyObject *key)
{
    PyObject *value;

    register size_t i;
    register size_t perturb;
    register size_t mask;
    register Entry *entry;
    Entry *entry_0; // register-ing this slows by 33%
    Py_hash_t hash;

    entry_0 = (Entry *)&self->entries[0];

    mask = self->size - 1;

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
    value = PyObject_CallFunctionObjArgs(self->function, key, NULL);

    if (value == NULL)
        return NULL;

    if (insert(self, entry, key, value) == -1)
        return NULL;

    return value;
}

int
Memoizer_assign(Memoizer *self, PyObject *key, PyObject *value)
{
    register size_t i;
    register size_t perturb;
    register size_t mask;
    Entry *entry_0;
    register Entry *entry;

    Py_hash_t hash = hash_int(key);

    mask = self->size - 1;

    entry_0 = (Entry *)&self->entries[0];

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

    return insert(self, entry, key, value);

  found:
    Py_DECREF(entry->value);

    if (value == NULL) {
        entry->key = NULL;
        entry->value = NULL;

        self->usable++;
    } else {
        Py_INCREF(value);

        entry->value = value;
    }

    return 0;
}

/* Type */

static PyMappingMethods as_mapping = {
    (lenfunc)__len__,           /* mp_length */
    (binaryfunc)Memoizer_get,    /* mp_subscript */
    (objobjargproc)Memoizer_assign, /* mp_ass_subscript */
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
    "b._types.Memoizer",       /* tp_name */
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
