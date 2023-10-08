#ifndef ctinyjson_H__
#define ctinyjson_H__

// json共有6种数据类型，使用枚举类型
typedef enum{
    tinyjson_NULL,tinyjson_FALSE,tinyjson_TRUE,tinyjson_NUMBER,
                tinyjson_STRING,tinyjson_ARRAY,tinyjson_OBJECT
}tinyjson_type;


//tinyjson的数据结构为树，每个节点用tinyjson_value表示
typedef struct{
    double n;
    tinyjson_type type;
}tinyjson_value;


enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG
};

int parse(tinyjson_value* v, const char* json);
tinyjson_type get_type(const tinyjson_value* v);
double get_number(const tinyjson_value* v);

#endif