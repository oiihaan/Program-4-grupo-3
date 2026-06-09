#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int _tests_run    = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;

#define ASSERT_EQ(expected, actual) do { \
    if ((int)(expected) != (int)(actual)) { \
        fprintf(stderr, "    linea %d: esperado %d, obtenido %d\n", \
                __LINE__, (int)(expected), (int)(actual)); \
        return 0; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "    linea %d: condicion falsa: %s\n", __LINE__, #cond); \
        return 0; \
    } \
} while(0)

#define ASSERT_FALSE(cond)  ASSERT_TRUE(!(cond))

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, "    linea %d: esperado \"%s\", obtenido \"%s\"\n", \
                __LINE__, (expected), (actual)); \
        return 0; \
    } \
} while(0)

#define RUN_TEST(fn, desc) do { \
    _tests_run++; \
    if (fn()) { \
        _tests_passed++; \
    } else { \
        _tests_failed++; \
        fprintf(stderr, "  FALLO: %s\n", desc); \
    } \
} while(0)

#define SECTION(title) /* noop */

#define PRINT_RESULTS() do { \
    if (_tests_failed == 0) \
        printf("%d tests OK\n", _tests_run); \
    else \
        fprintf(stderr, "%d/%d tests fallaron\n", _tests_failed, _tests_run); \
} while(0)

#define SILENT_CALL(ret_var, expr) do { \
    FILE *_null = fopen("/dev/null", "w"); \
    FILE *_old  = stdout; \
    stdout = _null; \
    ret_var = (expr); \
    stdout = _old; \
    fclose(_null); \
} while(0)

#define SILENT(expr) do { \
    FILE *_null = fopen("/dev/null", "w"); \
    FILE *_old  = stdout; \
    stdout = _null; \
    (expr); \
    stdout = _old; \
    fclose(_null); \
} while(0)

#endif /* TEST_FRAMEWORK_H */
