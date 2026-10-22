// Public SQLite 3.30.1 calls used by the menu units; the embedded library is C.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
int sqlite3_open(const char *, sqlite3 **);
int sqlite3_close(sqlite3 *);
int sqlite3_exec(sqlite3 *, const char *, int (*)(void *, int, char **, char **), void *, char **);
void sqlite3_free(void *);
int sqlite3_prepare_v2(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
int sqlite3_bind_text(sqlite3_stmt *, int, const char *, int, void (*)(void *));
int sqlite3_bind_int(sqlite3_stmt *, int, int);
int sqlite3_step(sqlite3_stmt *);
int sqlite3_column_int(sqlite3_stmt *, int);
int sqlite3_column_type(sqlite3_stmt *, int);
const unsigned char *sqlite3_column_text(sqlite3_stmt *, int);
int sqlite3_finalize(sqlite3_stmt *);
const char *sqlite3_errmsg(sqlite3 *);
#ifdef __cplusplus
}
#endif
