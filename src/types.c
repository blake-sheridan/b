#include <Python.h>

#include "memoizer.h"

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Memoizer *memoizer;
} LazyProperty;

PyDoc_STRVAR(LazyProperty__doc__,
"TODO property __doc__");

extern PyTypeObject MemoizerType;

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

static PyGetSetDef getset[] = {
    {"__doc__",      LazyProperty__doc__getter},
    {"__name__",     LazyProperty__name__getter},
    {"__qualname__", LazyProperty__qualname__getter},
    {0}
};

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

    return Memoizer_get(this->memoizer, instance);
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

    return Memoizer_assign(this->memoizer, instance, value);
}

PyTypeObject LazyPropertyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "property",                /* tp_name */
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
    getset,                    /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    LazyProperty__get__,       /* tp_descr_get */
    LazyProperty__set__,       /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    LazyProperty__new__,       /* tp_new */
};

/* StructSequenceField */

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
} StructSequenceField;

typedef struct {
    PyDictObject dict;
    PyObject *fields;
} StructSequenceNamespace;

typedef struct {
    PyHeapTypeObject heap_type;
    Py_ssize_t num_fields;
    PyObject *fields;
} StructSequenceMeta;

typedef struct {
    PyObject_VAR_HEAD
    PyObject *values[1];
} StructSequence;

typedef struct {
    PyObject_HEAD
    Py_ssize_t index;
    StructSequence *sequence;
} StructSequenceIterator;

static PyTypeObject StructSequenceMeta_type;

static PyObject *
StructSequenceField__new__(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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

    StructSequenceField *this = (StructSequenceField *)self;

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
StructSequenceField__del__(PyObject *self)
{
    StructSequenceField *this = (StructSequenceField *)self;

    Py_XDECREF(this->default_);
    Py_XDECREF(this->name);
    Py_XDECREF(this->owner_qualname);
    Py_XDECREF(this->qualname);

    Py_DECREF(this->type);

    Py_TYPE(self)->tp_free(self);
}

static PyObject *
StructSequenceField__doc__(PyObject *self, void *_)
{
    PyObject *retval = ((StructSequenceField *)self)->doc;

    if (retval == NULL)
        retval = Py_None;

    Py_INCREF(retval);

    return retval;
}

static PyObject *
StructSequenceField__name__(PyObject *self, void *_)
{
    PyObject *retval = ((StructSequenceField *)self)->name;

    if (retval == NULL) {
        /* If accessed before even setting into classdef locals... */
        PyErr_SetString(PyExc_AttributeError, "__name__");
        return NULL;
    }

    Py_INCREF(retval);

    return retval;
}

static PyObject *
StructSequenceField__qualname__(PyObject *self, void *_)
{
    StructSequenceField *this = (StructSequenceField *)self;
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
StructSequenceField__get__(PyObject *self, PyObject *instance, PyObject *owner)
{
    if (instance == NULL || instance == Py_None) {
        Py_INCREF(self);
        return self;
    }

/*
    if (!PyObject_TypeCheck(instance, &StructSequenceMeta_type)) {
        PyErr_Format(PyExc_TypeError, "Expecting StructSequence, got: %R",
                     instance);
        return NULL;
    }
*/
    Py_ssize_t index = ((StructSequenceField *)self)->index;

    PyObject *value = ((StructSequence *)instance)->values[index];

    Py_INCREF(value);

    return value;
}

static int
StructSequenceField__set__(PyObject *self, PyObject *instance, PyObject *value)
{
    PyErr_SetString(PyExc_AttributeError, "StructSequences are immutable");
    return -1;
}

static PyGetSetDef
StructSequenceField_getset[] = {
    {"__doc__",      StructSequenceField__doc__,      NULL, "TODO"},
    {"__name__",     StructSequenceField__name__,     NULL, "TODO"},
    {"__qualname__", StructSequenceField__qualname__, NULL, "TODO"},
    {0}
};

static PyTypeObject
StructSequenceField_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.StructSequenceField",  /* tp_name */
    sizeof(StructSequenceField),    /* tp_basicsize */
    0,                              /* tp_itemsize */
    StructSequenceField__del__,     /* tp_dealloc */
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
    StructSequenceField_getset,     /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    StructSequenceField__get__,     /* tp_descr_get */
    StructSequenceField__set__,     /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    StructSequenceField__new__,     /* tp_new */
};

/* StructSequenceNamespace */

static void
StructSequenceNamespace__del__(PyObject *self)
{
    Py_DECREF(((StructSequenceNamespace *)self)->fields);
    PyDict_Type.tp_dealloc(self);
}

static int
StructSequenceNamespace__setitem__(PyObject *self, PyObject *key, PyObject *value)
{
    if (Py_TYPE(value) == &StructSequenceField_type) {
        PyObject *existing_value = PyDict_GetItem(self, key);
        if (existing_value != NULL) {
            PyErr_Format(PyExc_TypeError, "%R already assigned to %R",
                         key,
                         existing_value);
            return -1;
        }

        StructSequenceField *field = (StructSequenceField *)value;

        if (field->name != NULL) {
            PyErr_Format(PyExc_TypeError, "%R already assigned as %R",
                         field,
                         field->name);
            return -1;
        }

        PyObject *fields = ((StructSequenceNamespace *)self)->fields;

        Py_INCREF(key);

        field->name = key;

        if (PyList_Append(fields, value) == -1)
            return -1;

    }

    return PyDict_SetItem(self, key, value);
}

static int
StructSequenceNamespace_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
    if (value == NULL)
        return PyDict_DelItem(self, key);
    else
        return StructSequenceNamespace__setitem__(self, key, value);
}

static PyMappingMethods
StructSequenceNamespace_as_mapping = {
    0,                                     /* mp_length */
    0,                                     /* mp_subscript */
    StructSequenceNamespace_ass_subscript, /* mp_ass_subscript */
};

static PyTypeObject
StructSequenceNamespace_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.StructSequenceNamespace",   /* tp_name */
    sizeof(StructSequenceNamespace),     /* tp_basicsize */
    0,                                   /* tp_itemsize */
    StructSequenceNamespace__del__,      /* tp_dealloc */
    0,                                   /* tp_print */
    0,                                   /* tp_getattr */
    0,                                   /* tp_setattr */
    0,                                   /* tp_reserved */
    0,                                   /* tp_repr */
    0,                                   /* tp_as_number */
    0,                                   /* tp_as_sequence */
    &StructSequenceNamespace_as_mapping, /* tp_as_mapping */
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

/* StructSequenceMeta */

static PyObject *
StructSequenceMeta__prepare__(PyObject *self, PyObject *args)
{
    if (PyType_Ready(&StructSequenceField_type) < 0)
        return NULL;

    if (PyType_Ready(&StructSequenceNamespace_type) < 0)
        return NULL;

    PyObject *namespace = PyDict_Type.tp_new(&StructSequenceNamespace_type, NULL, NULL);
    if (namespace == NULL)
        return NULL;

    PyObject *fields = PyList_New(0);
    if (fields == NULL)
        return NULL;

    if (PyDict_SetItemString(namespace, FIELD_KEY, (PyObject *)&StructSequenceField_type) == -1)
        return NULL;

    ((StructSequenceNamespace *)namespace)->fields = fields;

    return namespace;
}

static PyObject *
StructSequenceMeta__new__(PyTypeObject *mcs, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) != 3) {
        PyErr_SetString(PyExc_TypeError, "StructSequenceMeta: expecting 3 arguments");
        return NULL;
    }

    PyObject *namespace = PyTuple_GET_ITEM(args, 2);
    if (Py_TYPE(namespace) != &StructSequenceNamespace_type) {
        PyErr_SetString(PyExc_TypeError, "StructSequenceMeta: expecting custom namespace");
        return NULL;
    }

    if (PyDict_DelItemString(namespace, FIELD_KEY) == -1)
        return NULL; /* Custom error? */

    PyObject *slots = PyTuple_New(0);
    if (PyDict_SetItemString(namespace, "__slots__", slots) == -1)
        return NULL;

    PyObject *cls = PyType_Type.tp_new(mcs, args, kwargs);
    if (cls == NULL)
        return NULL;

    PyObject *fields = PyList_AsTuple(((StructSequenceNamespace *)namespace)->fields);
    if (fields == NULL)
        return NULL;

    Py_ssize_t num_fields = PyTuple_GET_SIZE(fields);
    if (num_fields) {
        PyObject *owner_qualname = ((PyHeapTypeObject *)cls)->ht_qualname;
        StructSequenceField *field;
        Py_ssize_t i;

        for (i = 0; i < num_fields; i++) {
            field = (StructSequenceField *)PyTuple_GET_ITEM(fields, i);

            Py_XINCREF(owner_qualname);
            field->index = i;
            field->owner_qualname = owner_qualname;
        }
    }

    ((StructSequenceMeta *)cls)->fields = fields;
    ((StructSequenceMeta *)cls)->num_fields = num_fields;

    return cls;
}

static Py_ssize_t
StructSequenceMeta__len__(PyObject *cls)
{
    return ((StructSequenceMeta *)cls)->num_fields;
}

static PyObject *
StructSequenceMeta__getitem__(PyObject *cls, Py_ssize_t index)
{
    Py_ssize_t num_fields = ((StructSequenceMeta *)cls)->num_fields;

    if (index < 0 || index >= num_fields) {
        PyErr_SetString(PyExc_IndexError, "StructSequenceMeta index out of range");
        return NULL;
    }

    PyErr_SetString(PyExc_NotImplementedError, "StructSequenceMeta.__iter__");
    return NULL;
}

static PyObject *
StructSequenceMeta__iter__(PyObject *cls)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequenceMeta.__iter__");
    return NULL;
}

static PySequenceMethods
StructSequenceMeta_as_sequence = {
    StructSequenceMeta__len__,     /* sq_length */
    0,                             /* sq_concat */
    0,                             /* sq_repeat */
    StructSequenceMeta__getitem__, /* sq_item */
    0,                             /* sq_slice */
    0,                             /* sq_ass_item */
    0,                             /* sq_ass_slice */
    0,                             /* sq_contains */
};

static PyMethodDef
StructSequenceMeta_methods[] = {
    {"__prepare__", StructSequenceMeta__prepare__, METH_VARARGS | METH_CLASS, "TODO"},
    {NULL, NULL} /* sentinel */
};

static PyTypeObject
StructSequenceMeta_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.StructSequenceMeta",   /* tp_name */
    sizeof(StructSequenceMeta),     /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    &StructSequenceMeta_as_sequence,/* tp_as_sequence */
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
    StructSequenceMeta__iter__,     /* tp_iter */
    0,                              /* tp_iternext */
    StructSequenceMeta_methods,     /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    &PyType_Type,                   /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    StructSequenceMeta__new__,      /* tp_new */
};

/* StructSequence */

PyDoc_STRVAR(StructSequence__doc__,
"TODO");

static PyObject *
StructSequence__new__(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    assert(PyType_IsSubtype(cls, &StructSequenceMeta_type));

    StructSequenceMeta *cls_this = (StructSequenceMeta *)cls;

    Py_ssize_t num_fields = cls_this->num_fields;
    Py_ssize_t num_args = PyTuple_GET_SIZE(args);

    if (kwargs != NULL) {
        PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__new__, **kwargs");
        return NULL;
    }

    PyObject *self = cls->tp_alloc(cls, num_fields);
    if (self == NULL)
        return NULL;

    StructSequence *this = (StructSequence *)self;

    PyObject *value;
    Py_ssize_t i;
    for (i = 0; i < num_fields; i++) {
        if (i < num_args) {
            value = PyTuple_GET_ITEM(args, i);

            /* TODO: type check */

            Py_INCREF(value);
            this->values[i] = value;
        } else {
            PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__new__, arg defaults");
            return NULL;
        }
    }

    return self;
}

static void
StructSequence__del__(PyObject *self)
{
    Py_ssize_t i, n;
    for (i = 0, n = Py_SIZE(Py_TYPE(self)); i < n; i++) {
        Py_XDECREF(((StructSequence *)self)->values[i]);
    }

    Py_TYPE(self)->tp_free(self);
}

static Py_hash_t
StructSequence__hash__(PyObject *self)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__hash__");
    return -1;
}

static PyObject *
StructSequence__repr__(PyObject *self)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__repr__");
    return NULL;
}

/* StructSequence :: as container */

static int
StructSequence__contains__(PyObject *self, PyObject *value)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__contains__");
    return -1;
}

static PyObject *
StructSequence__getitem__(PyObject *self, Py_ssize_t index)
{
    StructSequenceMeta *this_cls = (StructSequenceMeta *)Py_TYPE((StructSequenceMeta *)self);
    Py_ssize_t num_fields = this_cls->num_fields;

    if (index < 0 || index >= num_fields) {
        PyErr_SetString(PyExc_IndexError, "StructSequence: index out of range");
        return NULL;
    }

    PyObject *value = ((StructSequence *)self)->values[index];
    Py_INCREF(value);
    return value;
}

static Py_ssize_t
StructSequence__len__(PyObject *self)
{
    return ((StructSequenceMeta *)Py_TYPE(self))->num_fields;
}

static PyTypeObject StructSequenceIterator_type;

static PyObject *
StructSequence__iter__(PyObject *self)
{
    if (PyType_Ready(&StructSequenceIterator_type) < 0)
        return NULL;

    StructSequenceIterator *retval = PyObject_New(StructSequenceIterator, &StructSequenceIterator_type);
    if (retval == NULL)
        return NULL;

    Py_INCREF(self);

    retval->index = 0;
    retval->sequence = (StructSequence *)self;

    return (PyObject *)retval;
}

/* StructSequence :: rich comparison */

static PyObject *
StructSequence__lt__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__lt__");
    return NULL;
}

static PyObject *
StructSequence__le__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__le__");
    return NULL;
}

static PyObject *
StructSequence__eq__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__eq__");
    return NULL;
}

static PyObject *
StructSequence__ne__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__ne__");
    return NULL;
}

static PyObject *
StructSequence__gt__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__gt__");
    return NULL;
}

static PyObject *
StructSequence__ge__(PyObject *self, PyObject *other)
{
    PyErr_SetString(PyExc_NotImplementedError, "StructSequence.__ge__");
    return NULL;
}

static PyObject *
StructSequence_richcompare(PyObject *self, PyObject *other, int op)
{
    switch (op) {
    case Py_LT: return StructSequence__lt__(self, other);
    case Py_LE: return StructSequence__le__(self, other);
    case Py_EQ: return StructSequence__eq__(self, other);
    case Py_NE: return StructSequence__ne__(self, other);
    case Py_GT: return StructSequence__gt__(self, other);
    case Py_GE: return StructSequence__ge__(self, other);
    default: return NULL;
    }
}

/* StructSequence - type */

static PySequenceMethods
StructSequence_as_sequence = {
    StructSequence__len__,         /* sq_length */
    0,                             /* sq_concat */
    0,                             /* sq_repeat */
    StructSequence__getitem__,     /* sq_item */
    0,                             /* sq_slice */
    0,                             /* sq_ass_item */
    0,                             /* sq_ass_slice */
    StructSequence__contains__,    /* sq_contains */
};

static PyMethodDef
StructSequence_methods[] = {
    {NULL, NULL} /* sentinel */
};

static PyTypeObject
StructSequence_type = {
    PyVarObject_HEAD_INIT(&StructSequenceMeta_type, 0)
    "b.types.StructSequence",       /* tp_name */
    sizeof(StructSequence) - sizeof(PyObject *), /* tp_basicsize */
    sizeof(PyObject *),             /* tp_itemsize */
    StructSequence__del__,          /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    StructSequence__repr__,         /* tp_repr */
    0,                              /* tp_as_number */
    &StructSequence_as_sequence,    /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    StructSequence__hash__,         /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,        /* tp_flags */
    StructSequence__doc__,          /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    StructSequence_richcompare,     /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    StructSequence__iter__,         /* tp_iter */
    0,                              /* tp_iternext */
    StructSequence_methods,         /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    StructSequence__new__,          /* tp_new */
};

/* StructSequenceIterator */

static void
StructSequenceIterator__del__(PyObject *self)
{
    Py_XDECREF(((StructSequenceIterator *)self)->sequence);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
StructSequenceIterator__next__(PyObject *self)
{
    StructSequenceIterator *this = (StructSequenceIterator *)self;
    StructSequence *sequence = this->sequence;

    if (sequence == NULL)
        return NULL;

    PyObject *retval = sequence->values[this->index];

    if (++this->index >= ((StructSequenceMeta *)Py_TYPE(sequence))->num_fields) {
        Py_DECREF(sequence);
        this->sequence = NULL;
    }

    Py_INCREF(retval);

    return retval;
}

static PyObject *
StructSequenceIterator__length_hint__(PyObject *self, PyObject *_)
{
    StructSequenceIterator *this = (StructSequenceIterator *)self;

    return PyLong_FromSsize_t(
        ((StructSequenceMeta *)Py_TYPE(this->sequence))->num_fields - this->index
        );
}

static PyMethodDef
StructSequenceIterator_methods[] = {
    {"__length_hint__", StructSequenceIterator__length_hint__, METH_NOARGS, NULL},
    {NULL, NULL}           /* sentinel */
};

static PyTypeObject
StructSequenceIterator_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b.types.StructSequenceIterator", /* tp_name */
    sizeof(StructSequenceIterator), /* tp_basicsize */
    0,                              /* tp_itemsize */
    StructSequenceIterator__del__,  /* tp_dealloc */
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
    PyObject_SelfIter,              /* tp_iter */
    StructSequenceIterator__next__, /* tp_iternext */
    StructSequenceIterator_methods, /* tp_methods */
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

    if (PyType_Ready(&LazyPropertyType) < 0)
        return NULL;

    Py_INCREF(&LazyPropertyType);

    PyModule_AddObject(module, "lazyproperty", (PyObject *)&LazyPropertyType);

    /* Memoizer */

    if (PyType_Ready(&MemoizerType) < 0)
        return NULL;

    Py_INCREF(&MemoizerType);

    PyModule_AddObject(module, "Memoizer", (PyObject *)&MemoizerType);

    /* StructSequenceMeta */

    if (PyType_Ready(&StructSequenceMeta_type) < 0)
        return NULL;

    Py_INCREF(&StructSequenceMeta_type);

    PyModule_AddObject(module, "_StructSequenceMeta", (PyObject *)&StructSequenceMeta_type);

    /* StructSequence */

    if (PyType_Ready(&StructSequence_type) < 0)
        return NULL;

    Py_INCREF(&StructSequence_type);

    PyModule_AddObject(module, "StructSequence", (PyObject *)&StructSequence_type);

    return module;
};
