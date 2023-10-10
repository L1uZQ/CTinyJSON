#ifndef ctinyjson_H__
#define ctinyjson_H__
#include <stddef.h> // size_t ->目标平台下数组的最大尺寸

// json共有6种数据类型，使用枚举类型
typedef enum{
    tinyjson_NULL,tinyjson_FALSE,tinyjson_TRUE,tinyjson_NUMBER,
                tinyjson_STRING,tinyjson_ARRAY,tinyjson_OBJECT
}tinyjson_type;


//tinyjson的数据结构为树，每个节点用tinyjson_value表示
typedef struct{
    /// @brief 一个值不可能同时为字符串和数字，因此用union定义节省内存
    union{
        struct {char *s; size_t len;}s; //字符串
        double n;
    }u; 
    tinyjson_type type;
}tinyjson_value;

enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR
};

#define set_null(v) tinyjson_free(v)
#define init(v) do { (v)->type = tinyjson_NULL; } while(0)


int parse(tinyjson_value* v, const char* json);
tinyjson_type get_type(const tinyjson_value* v);
double get_number(const tinyjson_value* v);

int get_boolean(const tinyjson_value* v);
void set_boolean(tinyjson_value* v, int b);

double get_number(const tinyjson_value* v);
void set_number(tinyjson_value* v, double n);

const char* get_string(const tinyjson_value* v);
size_t get_string_length(const tinyjson_value* v);
void set_string(tinyjson_value* v, const char* s, size_t len);

#endif