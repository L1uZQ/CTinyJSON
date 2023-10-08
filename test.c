#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctinyjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

#define TEST_ERROR(error, json)\
    do {\
        tinyjson_value v;\
        v.type = tinyjson_FALSE;\
        EXPECT_EQ_INT(error, parse(&v, json));\
        EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));\
    } while(0)


//测试数字时用宏减少代码量
#define TEST_NUMBER(expect, json)\
    do {\
        tinyjson_value v;\
        EXPECT_EQ_INT(PARSE_OK, parse(&v, json));\
        EXPECT_EQ_INT(tinyjson_NUMBER,get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, get_number(&v));\
    } while(0)

static void test_parse_null() {
    tinyjson_value v;
    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_OK, parse(&v, "null"));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));
}

static void test_parse_expect_value() {
    tinyjson_value v;

    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_EXPECT_VALUE, parse(&v, ""));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));

    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_EXPECT_VALUE, parse(&v, " "));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));
}

static void test_parse_invalid_value() {
    tinyjson_value v;
    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_INVALID_VALUE, parse(&v, "nul"));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));

    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_INVALID_VALUE, parse(&v, "?"));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));

    #if 0
        /* ... */
    /* invalid number */
    TEST_ERROR(PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, "nan");
    #endif
}

static void test_parse_root_not_singular() {
    tinyjson_value v;
    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_ROOT_NOT_SINGULAR, parse(&v, "null x"));
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));
}

static void test_parse_true(){
    tinyjson_value v;
    v.type = tinyjson_FALSE;
    EXPECT_EQ_INT(PARSE_OK,parse(&v,"true"));
    EXPECT_EQ_INT(tinyjson_TRUE,get_type(&v));
}

static void test_parse_false(){
    tinyjson_value v;
    v.type = tinyjson_TRUE;
    EXPECT_EQ_INT(PARSE_OK,parse(&v,"false"));
    EXPECT_EQ_INT(tinyjson_FALSE,get_type(&v));
}

static void test_parse_number(){
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}



static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}