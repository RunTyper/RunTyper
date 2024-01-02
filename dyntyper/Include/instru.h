//
// Created by yanggw2022 on 18/03/23.
//

#ifndef INSTRU_H
#define INSTRU_H

#include "Python.h"

#include "code.h"
#include "dictobject.h"
#include "frameobject.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXPORT_SQL 1
// #define EXPORT_STD 1

void instru_enter_program(void);
void instru_exit_program(void);

void before_store_fast(int, PyObject*, PyCodeObject*);
void after_store_fast(int, PyObject*, PyCodeObject*);
void before_store_subscr(PyObject*, PyObject*, PyObject*, PyCodeObject*);
void after_store_subscr(PyObject*, PyObject*, PyObject*, PyCodeObject*);
void before_store_name(PyObject*, PyObject*, PyCodeObject*);
void after_store_name(PyObject*, PyObject*, PyCodeObject*);
void before_store_attr(PyObject*, PyObject*, PyObject*, PyCodeObject*, PyThreadState*);
void after_store_attr(PyObject*, PyObject*, PyObject*, PyCodeObject*, PyThreadState*);
void before_store_global(PyObject*, PyObject*, PyCodeObject*);
void after_store_global(PyObject*, PyObject*, PyCodeObject*);
void before_store_deref(int, PyObject*, PyCodeObject*);
void after_store_deref(int, PyObject*, PyCodeObject*);

void before_return_value(PyObject*, PyFrameObject*, PyCodeObject*);

void before_call_function(PyObject**, Py_ssize_t, PyCodeObject*, PyFrameObject*);
void after_call_function(PyObject**, Py_ssize_t, PyCodeObject*, PyFrameObject*);
void before_call_method(PyObject**, Py_ssize_t, PyCodeObject*, PyFrameObject*);
void after_call_method(PyObject**, Py_ssize_t, PyCodeObject*, PyFrameObject*);

void before_load_fast(int, PyObject*, PyCodeObject*);
void after_load_fast(int, PyObject*, PyCodeObject*);
void before_load_name(PyObject*, PyObject*, PyCodeObject*);
void after_load_name(PyObject*, PyObject*, PyCodeObject*);
void before_load_global(PyObject*, PyObject*, PyCodeObject*);
void after_load_global(PyObject*, PyObject*, PyCodeObject*);
void before_load_closure(int, PyObject*, PyCodeObject*);
void after_load_closure(int, PyObject*, PyCodeObject*);
void before_load_deref(int, PyObject*, PyCodeObject*);
void after_load_deref(int, PyObject*, PyCodeObject*);
void before_load_classderef(PyObject*, PyObject*, PyCodeObject*);
void after_load_classderef(PyObject*, PyObject*, PyCodeObject*);
void before_load_attr(PyObject*, PyObject*, PyObject*, PyCodeObject*, PyThreadState*);
void after_load_attr(PyObject*, PyObject*, PyObject*, PyCodeObject*, PyThreadState*);

#ifdef __cplusplus
}
#endif

#endif //PY36_INSTR_H
