#ifndef EXPORT_SQL_H
#define EXPORT_SQL_H

#ifndef _mysql_h
#include <mariadb/mysql.h>
#endif

#include <stdarg.h>

const char* SQL_HOST = "10.19.127.246";
const char* SQL_UNAME = "codergwy";
const char* SQL_PASSWD = "yangguowei";
const char* SQL_DB = "cpython";
const int SQL_PORT = 3306;

const int MAX_TRY_NUM = 10;

static MYSQL *sql_con = NULL;

void sqlerr(MYSQL* con, int exit_now) {
    fprintf(stderr, "MYSQL ERROR: %s\n", mysql_error(con));
    if (exit_now) {
        mysql_close(con);
        con = NULL;
        exit(1);
    }
}

void try_connect(MYSQL* con) {
    int trynum = 0;
    while (mysql_real_connect(con, SQL_HOST, SQL_UNAME, SQL_PASSWD,
                              SQL_DB, SQL_PORT, NULL, 0) == NULL) {
        trynum++;
        if (trynum >= MAX_TRY_NUM) {
            sqlerr(con, 1);
        } else {
            sqlerr(con, 0);
        }
    }
}

/**
 * 目前问题：可能mysql连接太多导致中途报错退出
 * 修改方向： 1. insert失败不退出（可能导致漏记不少信息） 2. 改为全局 connection
 */

static char *QUERY_INSERT_LOCAL = "INSERT INTO vars (name, type, file, func_id) VALUES ('%s', 'local', '%s', %d)";
static char *QUERY_INSERT_GLOBAL = "INSERT INTO vars (name, type, file) VALUES ('%s', 'global', '%s')";
static char *QUERY_INSERT_ATTR = "INSERT INTO vars (name, type, file, class) VALUES ('%s', 'attribute', '%s', '%s')";
static char *QUERY_INSERT_TYPE = "INSERT INTO types (name) VALUES ('%s')";
static char *QUERY_INSERT_VAR_TYPE = "INSERT INTO var_type (var_id, type_id, file, lineno) VALUES (%d, %d, '%s', %d)";
static char *QUERY_INSERT_FUNC = "INSERT INTO funcs (name, file, lineno) VALUES ('%s', '%s', %d)";
static char *QUERY_INSERT_METHOD = "INSERT INTO funcs (name, type, file, lineno, class) VALUES ('%s', 'method', '%s', %d, '%s')";
static char *QUERY_INSERT_ARG = "INSERT INTO args (name, func_id) VALUES ('%s', %d)";
static char *QUERY_INSERT_ARG_TYPE = "INSERT INTO arg_type (arg_id, type_id, file, lineno) VALUES (%d, %d, '%s', %d)";
static char *QUERY_INSERT_FUNC_RET_TYPE = "INSERT INTO func_ret_type (func_id, type_id, file, lineno) VALUES (%d, %d, '%s', %d)";

static char *QUERY_SELECT_LOCAL = "SELECT id FROM vars WHERE name='%s' and type='local' and file='%s' and func_id=%d";
static char *QUERY_SELECT_GLOBAL = "SELECT id FROM vars WHERE name='%s' and type='global' and file='%s'";
static char *QUERY_SELECT_ATTR = "SELECT id FROM vars WHERE name='%s' and type='attribute' and file='%s' and class='%s'";
static char *QUERY_SELECT_TYPE = "SELECT id FROM types WHERE name='%s'";
static char *QUERY_SELECT_VAR_TYPE = "SELECT id FROM var_type WHERE var_id=%d and type_id=%d and file='%s' and lineno=%d";
static char *QUERY_SELECT_FUNC = "SELECT id FROM funcs WHERE name='%s' and file='%s' and lineno=%d";
static char *QUERY_SELECT_METHOD = "SELECT id FROM funcs WHERE name='%s' and type='method' and file='%s' and lineno=%d and class='%s'";
static char *QUERY_SELECT_ARG = "SELECT id FROM args WHERE name='%s' and func_id=%d";
static char *QUERY_SELECT_ARG_TYPE = "SELECT id FROM arg_type WHERE arg_id=%d and type_id=%d and file='%s' and lineno=%d";
static char *QUERY_SELECT_FUNC_RET_TYPE = "SELECT id FROM func_ret_type WHERE func_id=%d, type_id=%d and file='%s' and lineno=%d";

static char *QUERY_UPDATE_METHOD = "UPDATE funcs SET type='method', class='%s' WHERE id=%d";

/** USAGE IN STORE_LOCAL:
 * int func_id = sql_insert_func(...);
 * sql_insert_local(..., func_id);
 */

static int _sql_query_idx(MYSQL *con, char *qstr) {
    int ret = -1;

    if (mysql_query(con, qstr) == 0) {
        MYSQL_RES *res = mysql_store_result(con);
        if (res != NULL) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row) {
                ret = atoi(row[0]);
            }
        }
        mysql_free_result(res);
    }
    return ret;
}

static int _sql_insert(MYSQL *con, char *qstr) {
    int trynum = 0;
    while (mysql_query(con, qstr)) {
        trynum++;
        if (trynum >= MAX_TRY_NUM) {
            sqlerr(con, 1);
        } else {
            sqlerr(con, 0);
        }
    }
    return (int)mysql_insert_id(con);
}

static int _sql_insert_any(char* fmt_sel, char* fmt_ins, ...) {
    int idx;
    char qstr[4096];

    if (sql_con == NULL) {
        sql_con = mysql_init(NULL);
        if (sql_con == NULL) {
            sqlerr(sql_con, 1);
        }
        try_connect(sql_con);
    }

    va_list args1, args2;
    va_start(args1, fmt_ins);
    va_copy(args2, args1);
    vsprintf(qstr, fmt_sel, args1);
    if ((idx = _sql_query_idx(sql_con, qstr)) >= 0) {
        // mysql_close(con);
        return idx;
    }
    vsprintf(qstr, fmt_ins, args2);
    idx = _sql_insert(sql_con, qstr);
    // mysql_close(con);
    return idx;
}

int sql_insert_func(char* name, char* file, int lineno) {
    return _sql_insert_any(QUERY_SELECT_FUNC, QUERY_INSERT_FUNC, name, file, lineno);
}

int sql_insert_method(char* name, char* file, int lineno, char* cls) {
    int idx;
    char qstr[4096];

    if (sql_con == NULL) {
        sql_con = mysql_init(NULL);
        if (sql_con == NULL) {
            sqlerr(sql_con, 1);
        }
        try_connect(sql_con);
    }
    sprintf(qstr, QUERY_SELECT_METHOD, name, file, lineno, cls);
    if ((idx = _sql_query_idx(sql_con, qstr)) >= 0) {
        return idx;
    }
    sprintf(qstr, QUERY_SELECT_FUNC, name, file, lineno);
    if ((idx = _sql_query_idx(sql_con, qstr)) >= 0) {
        sprintf(qstr, QUERY_UPDATE_METHOD, cls, idx);
        return _sql_insert(sql_con, qstr);
    }
    sprintf(qstr, QUERY_INSERT_METHOD, name, file, lineno, cls);
    return _sql_insert(sql_con, qstr);
}

int sql_insert_attr(char* name, char* file, char* cls) {
    return _sql_insert_any(QUERY_SELECT_ATTR, QUERY_INSERT_ATTR, name, file, cls);
}

int sql_insert_local(char* name, char* file, int func_id) {
    return _sql_insert_any(QUERY_SELECT_LOCAL, QUERY_INSERT_LOCAL, name, file, func_id);
}

int sql_insert_global(char* name, char* file) {
    return _sql_insert_any(QUERY_SELECT_GLOBAL, QUERY_INSERT_GLOBAL, name, file);
}

int sql_insert_type(char* name) {
    return _sql_insert_any(QUERY_SELECT_TYPE, QUERY_INSERT_TYPE, name);
}

int sql_insert_var_type(int var_id, int type_id, char* file, int lineno) {
    return _sql_insert_any(QUERY_SELECT_VAR_TYPE, QUERY_INSERT_VAR_TYPE, var_id, type_id, file, lineno);
}

int sql_insert_arg(char *name, int func_id) {
    return _sql_insert_any(QUERY_SELECT_ARG, QUERY_INSERT_ARG, name, func_id);
}

int sql_insert_arg_type(int arg_id, int type_id, char *file, int lineno) {
    return _sql_insert_any(QUERY_SELECT_ARG_TYPE, QUERY_INSERT_ARG_TYPE, arg_id, type_id, file, lineno);
}

int sql_insert_func_ret_type(int func_id, int type_id, char *file, int lineno) {
    return _sql_insert_any(QUERY_SELECT_FUNC_RET_TYPE, QUERY_INSERT_FUNC_RET_TYPE, func_id, type_id, file, lineno);
}

const int CREATE_TBL_QUERY_NUM = 7;
char *CREATE_TBL_QUERY[] = {
        "CREATE TABLE IF NOT EXISTS funcs("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  name VARCHAR(255) NOT NULL,"
        "  file VARCHAR(4096) NOT NULL,"
        "  lineno INT NOT NULL,"
        "  type ENUM('function', 'method') NOT NULL DEFAULT 'function',"
        "  class VARCHAR(255))",

        "CREATE TABLE IF NOT EXISTS vars("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  name VARCHAR(255) NOT NULL,"
        "  file VARCHAR(4096) NOT NULL,"
        "  type ENUM('local', 'global', 'attribute') NOT NULL DEFAULT 'global',"
        "  class VARCHAR(255),"
        "  func_id INT,"
        "  FOREIGN KEY (func_id) REFERENCES funcs(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE)",

        "CREATE TABLE IF NOT EXISTS types("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  name VARCHAR(255) NOT NULL)",

        "CREATE TABLE IF NOT EXISTS var_type("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  var_id INT,"
        "  FOREIGN KEY (var_id) REFERENCES vars(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  type_id INT,"
        "  FOREIGN KEY (type_id) REFERENCES types(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  file VARCHAR(4096) NOT NULL,"
        "  lineno INT NOT NULL)",

        "CREATE TABLE IF NOT EXISTS args("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  name VARCHAR(255) NOT NULL,"
        "  func_id INT NOT NULL,"
        "  FOREIGN KEY (func_id) REFERENCES funcs(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE)",

        "CREATE TABLE IF NOT EXISTS arg_type("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  arg_id INT,"
        "  FOREIGN KEY (arg_id) REFERENCES args(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  type_id INT,"
        "  FOREIGN KEY (type_id) REFERENCES types(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  file VARCHAR(4096) NOT NULL,"
        "  lineno INT NOT NULL)",

        "CREATE TABLE IF NOT EXISTS func_ret_type("
        "  id INT PRIMARY KEY AUTO_INCREMENT,"
        "  func_id INT,"
        "  FOREIGN KEY (func_id) REFERENCES funcs(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  type_id INT,"
        "  FOREIGN KEY (type_id) REFERENCES types(id)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE,"
        "  file VARCHAR(4096) NOT NULL,"
        "  lineno INT NOT NULL)"
};

static int _do_sql_create_db(MYSQL *con, char* qstr) {
    sprintf(qstr, "CREATE DATABASE IF NOT EXISTS %s", SQL_DB);
    if (mysql_query(con, qstr)) {
        sqlerr(con, 0);
        return -1;
    }
    sprintf(qstr, "USE %s", SQL_DB);
    if (mysql_query(con, qstr)) {
        sqlerr(con, 0);
        return -1;
    }
    for (int i = 0; i < CREATE_TBL_QUERY_NUM; i++) {
        if (mysql_query(con, CREATE_TBL_QUERY[i])) {
            sqlerr(con, 0);
            return -1;
        }
    }
    return 0;
}

void sql_create_db(MYSQL *con) {
    char qstr[4096];

    con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "MYSQL ERROR: %s\n", mysql_error(con));
    }
    if (mysql_real_connect(con, SQL_HOST, SQL_UNAME, SQL_PASSWD,
                           NULL, SQL_PORT, NULL, 0) == NULL) {
        sqlerr(con, 1);
    }
    if (mysql_select_db(con, SQL_DB)) {
        unsigned int err_num = mysql_errno(con);
        if (err_num == 1049) {
            int num = 0;
            while(_do_sql_create_db(con, qstr)) {
                num++;
                if (num >= MAX_TRY_NUM) {
                    sqlerr(con, 1);
                }
            }

        } else {
            sqlerr(con, 1);
        }
    }
    mysql_close(con);
    con = NULL;
}

void _sql_con_create_db(MYSQL *con) {
    char qstr[4096];

    con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "MYSQL ERROR: %s\n", mysql_error(con));
    }
    if (mysql_real_connect(con, SQL_HOST, SQL_UNAME, SQL_PASSWD,
                           NULL, SQL_PORT, NULL, 0) == NULL) {
        sqlerr(con, 1);
    }
    if (mysql_select_db(con, SQL_DB)) {
        unsigned int err_num = mysql_errno(con);
        if (err_num == 1049) {
            int num = 0;
            while(_do_sql_create_db(con, qstr)) {
                num++;
                if (num >= MAX_TRY_NUM) {
                    sqlerr(con, 1);
                }
            }
        } else {
            sqlerr(con, 1);
        }
    }
}

void init_sql_con() {
    _sql_con_create_db(sql_con);
}

void close_sql_con() {
    if (sql_con != NULL) {
        mysql_close(sql_con);
        sql_con = NULL;
    }
}

#endif
