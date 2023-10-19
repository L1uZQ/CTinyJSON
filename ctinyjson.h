#ifndef ctinyjson_H__
#define ctinyjson_H__
#include <stddef.h> // size_t ->目标平台下数组的最大尺寸

// JSON6种数据类型
typedef enum{
    tinyjson_NULL,tinyjson_FALSE,tinyjson_TRUE,tinyjson_NUMBER,
                tinyjson_STRING,tinyjson_ARRAY,tinyjson_OBJECT
}tinyjson_type;

#define KEY_NOT_EXIST ((size_t)-1)

typedef struct tinyjson_value tinyjson_value;
typedef struct tinyjson_member tinyjson_member;


/// @brief json树形结构,通过嵌套结构体表示，每个节点用tinyjson_value表示
struct tinyjson_value{
    /// @brief 一个值不可能同时为字符串和数字，因此用union定义节省内存
    union{
        struct {char *s; size_t len;}s; //字符串
        struct {tinyjson_value *e; size_t size, capacity;}a; //数组
        struct {tinyjson_member *m; size_t size, capacity;}o;
        double n;
    }u; 
    tinyjson_type type;
};

/// @brief 键值对结构
struct tinyjson_member{
    char *k;  //key string and klen
    size_t klen;
    tinyjson_value v;
};

/// @brief  JSON解析状态码
enum {
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR,
    PARSE_INVALID_UNICODE_HEX,
    PARSE_INVALID_UNICODE_SURROGATE,
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    PARSE_MISS_KEY,
    PARSE_MISS_COLON,
    PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#define set_null(v) tinyjson_free(v)
#define tinyjson_init(v) do { (v)->type = tinyjson_NULL; } while(0)

int parse(tinyjson_value* v, const char* json);
char* stringify(const tinyjson_value* v, size_t* length);

void tinyjson_copy(tinyjson_value* dst,const tinyjson_value* src);
void tinyjson_move(tinyjson_value* dst,tinyjson_value* src);
void tinyjson_swap(tinyjson_value* lhs,tinyjson_value* rhs);
int is_equal(tinyjson_value* lhs,tinyjson_value* rhs);


void tinyjson_free(tinyjson_value* v);


tinyjson_type get_type(const tinyjson_value* v);

int get_boolean(const tinyjson_value* v);
void set_boolean(tinyjson_value* v, int b);

double get_number(const tinyjson_value* v);
void set_number(tinyjson_value* v, double n);

const char* get_string(const tinyjson_value* v);
size_t get_string_length(const tinyjson_value* v);
void set_string(tinyjson_value* v, const char* s, size_t len);

size_t get_array_size(const tinyjson_value* v);
tinyjson_value* get_array_element(const tinyjson_value* v, size_t index);
void set_array(tinyjson_value* v, size_t capacity);
size_t get_array_capacity(const tinyjson_value* v);
void reserve_array(tinyjson_value* v, size_t capacity);
void shrink_array(tinyjson_value* v);
void clear_array(tinyjson_value* v);
tinyjson_value* pushback_array_element(tinyjson_value* v);
void popback_array_element(tinyjson_value* v);
tinyjson_value* insert_array_element(tinyjson_value* v, size_t index);
void erase_array_element(tinyjson_value* v, size_t index, size_t count);



void set_object(tinyjson_value * v, size_t capacity);
size_t get_object_size(const tinyjson_value* v);
size_t get_object_capacity(const tinyjson_value* v);
const char* get_object_key(const tinyjson_value* v, size_t index);
size_t get_object_key_length(const tinyjson_value* v, size_t index);
tinyjson_value* get_object_value(const tinyjson_value* v, size_t index);
tinyjson_value* set_object_value(tinyjson_value* v, const char* key, size_t klen);
tinyjson_value* find_object_value(tinyjson_value* v, const char* key, size_t klen);
size_t find_object_index(const tinyjson_value* v, const char* key, size_t klen);

void remove_object_value(tinyjson_value* v, size_t index);
void shrink_object(tinyjson_value* v);
void clear_object(tinyjson_value* v);
#endif