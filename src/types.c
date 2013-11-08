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

    PyObject *name      = PyTuple_GET_ITEM(args, 0);
    PyObject *bases     = PyTuple_GET_ITEM(args, 1);
    PyObject *namespace = PyTuple_GET_ITEM(args, 2);

    /* Sanity */

    if (!PyTuple_CheckExact(bases)) {
        PyErr_SetString(PyExc_TypeError, "NamedTupleMeta: expecting tuple of bases");
        return NULL;
    }

    if (PyTuple_GET_SIZE(bases) != 1) {
        PyErr_SetString(PyExc_NotImplementedError, "NamedTupleMeta: multiple inheritance");
        return NULL;
    }

    if (PyTuple_GET_ITEM(bases, 0) != (PyObject *)&NamedTuple_type) {
        PyErr_SetString(PyExc_NotImplementedError, "NamedTupleMeta: asdf");
        return NULL;
    }

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
    Py_ssize_t num_fields = ((NamedTupleMeta *)cls)->num_fields;

    if (index < 0 || index >= num_fields) {
        PyErr_SetString(PyExc_IndexError, "NamedTupleMeta index out of range");
        return NULL;
    }

    PyErr_SetString(PyExc_NotImplementedError, "NamedTupleMeta.__iter__");
    return NULL;
}

static PyObject *
NamedTupleMeta__iter__(PyObject *cls)
{
    PyErr_SetString(PyExc_NotImplementedError, "NamedTupleMeta.__iter__");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "NamedTuple.__repr__");
    return NULL;
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
