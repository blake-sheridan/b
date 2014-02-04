#include "Python.h"

#include "unicode.h"

#define DEFAULT_SIZE 100

typedef enum {
    UNDECIDED,
    IDENTIFIER,
    INTEGER,
    TOKEN
} Kind;

typedef struct {
    Kind kind;
    void *data;
} Symbol;

typedef struct {
    PyObject_VAR_HEAD
    Symbol (*stack)[1];
} Parser;

static PyTypeObject Parser_type;

PyDoc_STRVAR(__doc__,
"TODO Parser.__doc__");

static PyObject *
__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t size = DEFAULT_SIZE;

    if (!PyArg_ParseTuple(args, "|n", &size))
        return NULL;

    PyObject *self = PyObject_GC_NewVar(Parser, &Parser_type, size);

    if (self == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    return self;
}

static PyObject *
parse(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *string;

    if (!PyArg_ParseTuple(args, "U", &string))
        return NULL;

    if (PyUnicode_READY(string) == -1)
        return NULL;

    Py_ssize_t string_length = PyUnicode_GET_LENGTH(string);
    int        string_kind   = PyUnicode_KIND(string);
    void      *string_data   = PyUnicode_DATA(string);

    Py_UCS4 c;
    int i;

    Symbol token = {.kind = UNDECIDED, .data=NULL};
    int escape = 0;

    for (i = 0; i < string_length; i++) {
        c = PyUnicode_READ(string_kind, string_data, i);

        switch (c) {
        case 0x22: // "
            if (escape) {

            } else {

            }

            break;

        case 0x27: // '
            if (escape) {

            } else {

            }

            break;

        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
            switch (token.kind) {
            case UNDECIDED:
                token.kind = INTEGER;
                token.data = c - 0x30;
                break;

            case INTEGER:
                token.data += (c - 0x30);
                break;

            }

        case 0x5c: // backslash
            if (escape) {
                printf("todo literal backslash\n");
                escape = 0;
            } else {
                escape = 1;
            }

            break;

        CASE__WHITE_SPACE:
            switch (token.kind) {
            case INTEGER:
            case IDENTIFIER:
                printf("whitespace new token\n");
                break;
            }

            break;

        CASE__XID_START:
            switch (token.kind) {
            case UNDECIDED:
                token.kind = IDENTIFIER;
                break;

            case INTEGER:
                printf("todo\n");
                break;
            }

            break;

        CASE__XID_CONTINUE__EXCLUDING__XID_START:
            switch (token.kind) {
            case UNDECIDED:
                printf("continue start?\n");
                break;

            case INTEGER:
                printf("continue integer\n");
                break;
            }

            break;

        default:
            printf("unclassified");
        }
    }

    PyErr_SetString(PyExc_NotImplementedError, "parse");
    return NULL;
}

static PyMethodDef
Parser_methods[] = {
    {"parse", parse, METH_VARARGS, "todo"},
    {NULL, NULL} /* sentinel */
};

static PyTypeObject
Parser_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "b._grammar.Parser",       /* tp_name */
    sizeof(Parser),            /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
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
    __doc__,                   /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Parser_methods,            /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    __new__,                   /* tp_new */
};

/* Module */

PyDoc_STRVAR(Module__doc__,
"TODO _grammar __doc__");

static struct PyModuleDef
Module = {
    PyModuleDef_HEAD_INIT,
    "b._grammar",
    Module__doc__,
    -1,
};

PyMODINIT_FUNC
PyInit__grammar(void)
{
    PyObject *module = PyModule_Create(&Module);

    if (module == NULL)
        return NULL;

    if (PyType_Ready(&Parser_type) < 0)
        return NULL;

    Py_INCREF(&Parser_type);

    PyModule_AddObject(module, "Parser", (PyObject *)&Parser_type);

    return module;
}
