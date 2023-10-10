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
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")


#define TEST_ERROR(error, json)\
    do {\
        tinyjson_value v;\
        v.type = tinyjson_FALSE;\
        EXPECT_EQ_INT(error, parse(&v, json));\
        EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));\
        tinyjson_free(&v);\
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

    #if 1
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

    //边界值测试
}


static void test_parse_number_too_big() {
#if 1
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}


#define TEST_STRING(expect, json)\
    do {\
        tinyjson_value v;\
        init(&v);\
        EXPECT_EQ_INT(PARSE_OK, parse(&v, json));\
        EXPECT_EQ_INT(tinyjson_STRING, get_type(&v));\
        EXPECT_EQ_STRING(expect, get_string(&v), get_string_length(&v));\
        tinyjson_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}


static void test_parse_missing_quotation_mark() {
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}


static void test_access_null() {
    tinyjson_value v;
    init(&v);
    set_string(&v, "a", 1);
    set_null(&v);
    EXPECT_EQ_INT(tinyjson_NULL, get_type(&v));
    tinyjson_free(&v);
}

static void test_access_boolean() {
    tinyjson_value v;
    init(&v);
    set_string(&v, "a", 1);
    set_boolean(&v, 1);
    EXPECT_TRUE(get_boolean(&v));
    set_boolean(&v, 0);
    EXPECT_FALSE(get_boolean(&v));
    tinyjson_free(&v);
}

static void test_access_number() {
    tinyjson_value v;
    init(&v);
    set_string(&v, "a", 1);
    set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, get_number(&v));
    tinyjson_free(&v);
}

static void test_access_string() {
    tinyjson_value v;
    init(&v);
    set_string(&v, "", 0);
    EXPECT_EQ_STRING("", get_string(&v), get_string_length(&v));
    set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", get_string(&v), get_string_length(&v));
    tinyjson_free(&v);
}


static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();

    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}