//
// Created by yanggw2022 on 18/03/23.
//
#include "Python.h"

#include "instru.h"
#include "export_sql.h"

#ifdef __cplusplus
//extern "C" {
#endif

int check_fname(char* fname) {
    if (fname == NULL) {
        return 0;
    }

    int len = strlen(fname);
    if (len > 0
        && strstr(fname, "cpython/Lib") == NULL
        && strstr(fname, "/usr/") == NULL
        && strstr(fname, "/pip/") == NULL
        && strstr(fname, "/site-packages/") == NULL
        && strstr(fname, "/dist/") == NULL
        && strstr(fname, "/_vendor/") == NULL
        && strstr(fname, "/_internal/") == NULL
        && strstr(fname, "/tmp/") == NULL
        && strstr(fname, "/setuptools/") == NULL
//        && strstr(fname, "/_pytest/") == NULL
//        && strstr(fname, "/tests/") == NULL
        && strstr(fname, "/importlib_metadata/") == NULL
        && strstr(fname, "/pkg_resources/") == NULL
        && (!(strstr(fname, "/lib/python3.") != NULL && strstr(fname, "/venv/") == NULL))
        && strstr(fname, "/Lib/") == NULL
        && strstr(fname, "/setup.py") == NULL
        && !(fname[0] == '<' && fname[len - 1] == '>')
            ) {
        return 1;
    }
    return 0;
}

int check_tpname(char *name) {
    if (name == NULL ||
        strcmp(name, "type") == 0 ||
        strcmp(name, "function") == 0||
        strcmp(name, "method") == 0
            ) {
        return 0;
    }
    return 1;
}

int check_vname(char *vname) {
    if (vname == NULL) {
        return 0;
    }
    if (vname == '_') {
        return 0;
    }
    int len = strlen(vname);
    if (len == 0) {
        return 0;
    }
    if (len > 0 && vname[0] == '@') {
        return 0;
    }
    if (len >= 2 && vname[0] == '<' && vname[len - 1] == '>') {
        return 0;
    }
    if (len >= 4 && vname[0] == '_' && vname[1] == '_' && vname[len - 2] == '_' && vname[len - 1] == '_') {
        return 0;
    }
    return 1;
}

char* get_cls_file(PyObject *obj, PyThreadState *tstate) {
    PyTypeObject* type = PyObject_Type(obj);
    if (! PyObject_HasAttrString((PyObject*)type, "__module__")) {
        return NULL;
    }
    PyObject *module_name = PyObject_GetAttrString((PyObject *) type, "__module__");

    // PyObject *modules = ((PyInterpreterState *)tstate->interp)->modules;
    PyObject *modules = PyImport_GetModuleDict();
    if (PyDict_Contains(modules, module_name) != 1) {
        return NULL;
    }
    PyObject *module = PyDict_GetItem(modules, module_name);
    if (! PyObject_HasAttrString(module, "__file__")) {
        return NULL;
    }
    PyObject *file_name = PyModule_GetFilenameObject(module);
    if (file_name == NULL) {
        return NULL;
    }
    return PyUnicode_AsUTF8(file_name);
}

void export_global(char* instru, PyObject* name, PyObject* v, PyCodeObject* co) {
    char *fname = PyUnicode_AsUTF8(co->co_filename);
    char *vname = PyUnicode_AsUTF8(name);
    char *tpname = v->ob_type->tp_name;

    if (!check_fname(fname) || !check_vname(vname) || !check_tpname(tpname)) return;

#ifdef EXPORT_STD
    printf("[%s](%s): %s, %s\n", instru, fname, vname, tpname);
#endif
#ifdef EXPORT_SQL
    int var_id = sql_insert_global(vname, fname);
    int tp_id = sql_insert_type(tpname);
    sql_insert_var_type(var_id, tp_id, fname, co->co_firstlineno);
#endif
}

void export_local(char* instru, int oparg, PyObject *value, PyCodeObject* co) {
    char* fname = PyUnicode_AsUTF8(co->co_filename);
    char* mname = PyUnicode_AsUTF8(co->co_name);
    char* vname = PyUnicode_AsUTF8(((PyTupleObject *) (co->co_varnames))->ob_item[oparg]);
    char* tpname = value->ob_type->tp_name;

    if (!check_fname(fname) || !check_vname(vname) || !check_tpname(tpname)) return;
#ifdef EXPORT_STD
    printf("[%s](%s): [%s]%s, %s\n", instru, fname, mname, vname, tpname);
#endif
#ifdef EXPORT_SQL
    int func_id = sql_insert_func(mname, fname, co->co_firstlineno);
    int var_id = sql_insert_local(vname, fname, func_id);
    int tp_id = sql_insert_type(tpname);
    sql_insert_var_type(var_id, tp_id, fname, co->co_firstlineno);
#endif
}

void export_closure(char* instru, int oparg, PyObject *cell, PyCodeObject* co) {
    PyObject *value = PyCell_Get(cell);
    if (value == NULL) {
        return;
    }
    char *fname = PyUnicode_AsUTF8(co->co_filename);
    char *mname = PyUnicode_AsUTF8(co->co_name);
    char *vname;
    Py_ssize_t cell_len = PyTuple_Size(co->co_cellvars);
    if (oparg < cell_len) {
        vname = PyUnicode_AsUTF8(((PyTupleObject *) (co->co_cellvars))->ob_item[oparg]);
    } else {
        vname = PyUnicode_AsUTF8(((PyTupleObject *) (co->co_freevars))->ob_item[oparg - cell_len]);
    }
    char *tpname = value->ob_type->tp_name;
    if (!check_fname(fname) || !check_vname(vname) || !check_tpname(tpname)) return;
#ifdef EXPORT_STD
    printf("[%s](%s): [%s]%s, %s\n", instru, fname, mname, vname, tpname);
#endif
#ifdef EXPORT_SQL
    int var_id = sql_insert_global(vname, fname);
    int tp_id = sql_insert_type(tpname);
    sql_insert_var_type(var_id, tp_id, fname, co->co_firstlineno);
#endif
}

void export_var_dict(PyDictObject *vars, PyCodeObject* co) {
    PyListObject *items = (PyListObject *) PyDict_Items(vars);
    for (int i = 0; i < PyList_GET_SIZE(items); i++) {
        PyTupleObject *item = PyList_GET_ITEM(items, i);
        PyObject *gname = PyTuple_GET_ITEM(item, 0);
        PyObject *gvalue = PyTuple_GET_ITEM(item, 1);
        if (check_vname(PyUnicode_AsUTF8(gname))) {
            export_global("CALL", gname, gvalue, co);
        }
    }
}

void export_attr(char* instru, PyObject* name, PyObject* owner, PyObject* v, PyCodeObject* co, PyThreadState *tstate) {
    char *cname =  owner->ob_type->tp_name;
    char *vname = PyUnicode_AsUTF8(name);
    char *tpname = v->ob_type->tp_name;
    if (!check_vname(cname) || !check_vname(vname) || !check_tpname(tpname)) return;
    char *fname = get_cls_file(owner, tstate);
    if (fname == NULL || !check_fname(fname)) return;

#ifdef EXPORT_STD
    printf("[%s](%s): (%s).%s, %s\n", instru, fname, cname, vname, tpname);
#endif
#ifdef EXPORT_SQL
    int var_id = sql_insert_attr(vname, fname, cname);
    int tp_id = sql_insert_type(tpname);
    sql_insert_var_type(var_id, tp_id, fname, co->co_firstlineno);
#endif
}

void export_func_args(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {
    PyObject **pfunc = sp - oparg - 1;
    PyObject *func = *pfunc;
    Py_ssize_t nargs = oparg - 0;
    PyObject **stack = sp - nargs;

    if (strcmp(Py_TYPE(func)->tp_name, "function") != 0) {
        return;
    }
    PyFunctionObject *func_obj = func;
    PyCodeObject *func_co = func_obj->func_code;
    if (func_co == NULL) {
        return;
    }
    Py_ssize_t num_args = func_co->co_argcount < oparg ? func_co->co_argcount : oparg;
    if (num_args == 0) {
        return;
    }
    PyUnicodeObject *fname_obj = co->co_filename;
    PyUnicodeObject *callee_fname_obj = func_co->co_filename;
    PyUnicodeObject *callee_name_obj = func_co->co_name;
    if (fname_obj == NULL || callee_fname_obj == NULL || callee_name_obj == NULL) {
        return;
    }
    char *fname = PyUnicode_AsUTF8(fname_obj);
    char *callee_fname = PyUnicode_AsUTF8(callee_fname_obj);
    char *callee_name = PyUnicode_AsUTF8(callee_name_obj);
    if (!check_fname(fname) || !check_fname(callee_fname) || !check_vname(callee_name)) {
        return;
    }

    int lineno = PyFrame_GetLineNumber(f);
    int callee_lineno = func_co->co_firstlineno;
    PyTupleObject *conames = func_co->co_varnames;

    export_var_dict((PyDictObject *) PyFunction_GetGlobals(func), func_co);
#ifdef EXPORT_STD
    printf("[CALL FUNCTION](%s:%d): %s(", callee_fname, callee_lineno, callee_name);
#endif
#ifdef EXPORT_SQL
    int func_id = sql_insert_func(callee_name, callee_fname, callee_lineno);
#endif
    for (Py_ssize_t i = 0; i < num_args; i++) {
        Py_ssize_t tem = oparg - i;
        PyObject *obj = *(sp - tem);
        char *argname = PyUnicode_AsUTF8(PyTuple_GetItem(conames, i));
        char *tpname = obj->ob_type->tp_name;
#ifdef EXPORT_STD
        printf("%s: %s", argname, tpname);
        if (i < num_args - 1) {
            printf(", ");
        }
#endif
#ifdef EXPORT_SQL
        int type_id = sql_insert_type(tpname);
        int arg_id = sql_insert_arg(argname, func_id);
        sql_insert_arg_type(arg_id, type_id, fname, lineno);
#endif
    }

#ifdef EXPORT_STD
    printf("), callsite: %s:%d\n", fname, lineno);
#endif
}

void export_method_args(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {
    PyObject **pfunc = sp - oparg - 2;
    PyObject *func = *pfunc;
    Py_ssize_t nargs = oparg - 0;
    PyObject **stack = sp - nargs;

    if (strcmp(Py_TYPE(func)->tp_name, "function") != 0) {
        return;
    }
    PyFunctionObject *func_obj = func;
    PyCodeObject *func_co = func_obj->func_code;
    if (func_co == NULL) {
        return;
    }
    // argcount includes the self, so need to -1
    Py_ssize_t num_args = (func_co->co_argcount - 1) < oparg ? (func_co->co_argcount - 1) : oparg;
    if (num_args == 0) {
        return;
    }
    PyUnicodeObject *fname_obj = co->co_filename;
    PyUnicodeObject *callee_fname_obj = func_co->co_filename;
    PyUnicodeObject *callee_name_obj = func_co->co_name;
    if (fname_obj == NULL || callee_fname_obj == NULL || callee_name_obj == NULL) {
        return;
    }
    char *fname = PyUnicode_AsUTF8(fname_obj);
    int lineno = PyFrame_GetLineNumber(f);
    char *callee_fname = PyUnicode_AsUTF8(callee_fname_obj);
    char *callee_name = PyUnicode_AsUTF8(callee_name_obj);
    int callee_lineno = func_co->co_firstlineno;
    if (!check_fname(fname) || !check_fname(callee_fname) || !check_vname(callee_name)) {
        return;
    }
    PyObject *self_obj = *(sp - oparg - 1);
    char *cls_name = self_obj->ob_type->tp_name;
    PyTupleObject *conames = func_co->co_varnames;

    export_var_dict((PyDictObject *) PyFunction_GetGlobals(func), func_co);
#ifdef EXPORT_STD
    printf("[CALL METHOD](%s:%d): %s.%s(", callee_fname, callee_lineno, cls_name, callee_name);
#endif
#ifdef EXPORT_SQL
    int func_id = sql_insert_method(callee_name, callee_fname, callee_lineno, cls_name);
#endif
    for (Py_ssize_t i = 0; i < num_args; i++) {
        Py_ssize_t tem = oparg - i;
        PyObject *obj = *(sp - tem);
        char *argname = PyUnicode_AsUTF8(PyTuple_GetItem(conames, i + 1));
        char *tpname = obj->ob_type->tp_name;
#ifdef EXPORT_STD
        printf("%s: %s", argname, tpname);
        if (i < num_args - 1) {
            printf(", ");
        }
#endif
#ifdef EXPORT_SQL
        int type_id = sql_insert_type(tpname);
        int arg_id = sql_insert_arg(argname, func_id);
        sql_insert_arg_type(arg_id, type_id, fname, lineno);
#endif
    }
#ifdef EXPORT_STD
    printf("), callsite: %s:%d\n", fname, lineno);
#endif
}


void export_ret(PyObject* retval, PyFrameObject* f, PyCodeObject* co) {
    char *fname = PyUnicode_AsUTF8(co->co_filename);
    char *vname = PyUnicode_AsUTF8(co->co_name);
    int lineno = co->co_firstlineno;
    char *tpname = retval->ob_type->tp_name;
    PyFrameObject *callerf = f->f_back;
    if (callerf == NULL || callerf->f_code == NULL || !check_fname(fname) || !check_vname(vname)) return;
    PyCodeObject *caller_co = callerf->f_code;
    char *caller_fname = PyUnicode_AsUTF8(caller_co->co_filename);
    int caller_lineno = PyFrame_GetLineNumber(callerf);
    if (caller_fname == NULL || !check_fname(caller_fname) || !check_fname) return;
#ifdef EXPORT_STD
    printf("[RETURN VALUE](%s:%d): %s, %s", fname, lineno, vname, tpname);
            printf("  (callsite: %s:%d)\n", caller_fname, caller_lineno);
#endif
#ifdef EXPORT_SQL
    int func_id = sql_insert_func(vname, fname, lineno);
    int type_id = sql_insert_type(tpname);
    sql_insert_func_ret_type(func_id, type_id, caller_fname, caller_lineno);
#endif
}


void instru_enter_program(void) {
    init_sql_con();
}
void instru_exit_program(void) {
    close_sql_con();
}

void before_store_fast(int oparg, PyObject* value, PyCodeObject* co) {
    export_local("STORE_FAST", oparg, value, co);
}
void after_store_fast(int oparg, PyObject* value, PyCodeObject* co) {}
void before_store_subscr(PyObject* sub, PyObject* container, PyObject* v, PyCodeObject* co) {}
void after_store_subscr(PyObject* sub, PyObject* container, PyObject* v, PyCodeObject* co) {}
void before_store_name(PyObject* name, PyObject* v, PyCodeObject* co) {
    export_global("STORE_NAME", name, v, co);
}
void after_store_name(PyObject* name, PyObject* v, PyCodeObject* co) {}

void before_store_attr(PyObject* name, PyObject* owner, PyObject* v, PyCodeObject* co, PyThreadState* tstate) {
    export_attr("STORE_ATTR", name, owner, v, co, tstate);
}

void after_store_attr(PyObject* name, PyObject* owner, PyObject* v, PyCodeObject* co, PyThreadState* tstate) {}
void before_store_global(PyObject* name, PyObject* v, PyCodeObject* co) {
    export_global("STORE_GLOBAL", name, v, co);
}
void after_store_global(PyObject* name, PyObject* v, PyCodeObject* co) {}
void before_store_deref(int oparg, PyObject *cell, PyCodeObject *co) {
    export_closure("STORE_DEREF(before)", oparg, cell, co);
}
void after_store_deref(int oparg, PyObject *cell, PyCodeObject *co) {
    export_closure("STORE_DEREF(after)", oparg, cell, co);
}

void before_return_value(PyObject* retval, PyFrameObject* f, PyCodeObject* co) {
    export_ret(retval, f, co);
}

void before_call_function(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {
    export_func_args(sp, oparg, co, f);
}
void after_call_function(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {}
void before_call_method(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {
    export_method_args(sp, oparg, co, f);
}
void after_call_method(PyObject **sp, Py_ssize_t oparg, PyCodeObject *co, PyFrameObject *f) {}


void before_load_fast(int oparg, PyObject *value, PyCodeObject *co) {
    export_local("LOAD_FAST", oparg, value, co);
}
void after_load_fast(int oparg, PyObject *value, PyCodeObject *co) {}
void before_load_name(PyObject *name, PyObject *v, PyCodeObject *co) {
    export_global("LOAD_NAME", name, v, co);
}
void after_load_name(PyObject *name, PyObject *v, PyCodeObject *co) {}
void before_load_global(PyObject *name, PyObject *v, PyCodeObject *co) {
    export_global("LOAD_GLOBAL", name, v, co);
}
void after_load_global(PyObject *name, PyObject *v, PyCodeObject *co) {}
void before_load_closure(int oparg, PyObject *cell, PyCodeObject *co) {
    export_closure("LOAD_CLOSURE", oparg, cell, co);
}
void after_load_closure(int oparg, PyObject *cell, PyCodeObject *co) {}
void before_load_deref(int oparg, PyObject *cell, PyCodeObject *co) {
    export_closure("LOAD_DEREF", oparg, cell, co);
}
void after_load_deref(int oparg, PyObject *cell, PyCodeObject *co) {}
void before_load_classderef(PyObject *name, PyObject *v, PyCodeObject *co) {
    export_global("LOAD_CLASSDEREF", name, v, co);
}
void after_load_classderef(PyObject *name, PyObject *v, PyCodeObject *co) {}

void before_load_attr(PyObject* name, PyObject* owner, PyObject* v, PyCodeObject* co, PyThreadState* tstate) {
    if (name == NULL || owner == NULL || v == NULL || co == NULL || tstate == NULL) return;
    export_attr("LOAD_ATTR", name, owner, v, co, tstate);
}

void after_load_attr(PyObject* name, PyObject* owner, PyObject* v, PyCodeObject* co, PyThreadState* tstate) {}

#ifdef __cplusplus
// }
#endif
