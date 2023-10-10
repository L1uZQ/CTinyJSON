#include"ctinyjson.h"
#include<assert.h>
#include<stdlib.h>
#include<errno.h>
#include<math.h>
#include<string.h>

#ifndef PARSE_STACK_INIT_SIZE
#define PARSE_STACK_INIT_SIZE 256
#endif


#define EXPECT(c, ch)   do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)   ((ch)>='0' && (ch)<='9')
#define ISDIGIT1TO9(ch)  ((ch)>='1' && (ch)<='9')

//通过赋值将ch存储在context_push函数返回的位置。
#define PUTC(c,ch)  do{*(char*)context_push(c,sizeof(char)) = (ch); } while(0)

/// @brief 
typedef struct{
    const char *json;
    char *stack; //动态堆栈
    size_t size,top;
}tinyjson_context;

static void* context_push(tinyjson_context* c, size_t size){
    void* ret;
    assert(size>0);
    if(c->top + size >= c->size){
        if(c->size==0){
            c->size = PARSE_STACK_INIT_SIZE;
        }
        while(c->top + size >= c->size) c->size += c->size >> 1; //扩充为原来的1.5倍

        c->stack = (char*) realloc(c->stack, c->size); //重新分配内存
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* context_pop(tinyjson_context* c, size_t size){
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}


/// @brief 解析空格
/// @param c 
static void parse_whitespace(tinyjson_context *c){
    const char *p = c->json;
    while(*p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
        p++;
    c->json = p;
}


/// @brief null、false、true的解析合成一个函数，通过传入参数区分
/// @param c 
/// @param v 
/// @param literal 
/// @param type 
/// @return 
static int parse_literal(tinyjson_context* c, tinyjson_value* v,
    const char* literal,tinyjson_type type)
{
    size_t i;
    EXPECT(c,literal[0]);
    for(i=0; literal[i+1]; i++){ //for循环直到'\0'
        if(c->json[i] != literal[i+1])
            return PARSE_INVALID_VALUE;
    }
    c->json += i;
    v->type = type;
    return PARSE_OK;
}

// static int parse_null(tinyjson_context* c, tinyjson_value* v){
//     EXPECT(c,'n');
//     if(c->json[0] !='u' || c->json[1]!='l' || c->json[2]!='l'){
//         return PARSE_INVALID_VALUE;
//     }
//     c->json += 3;
//     v->type =tinyjson_NULL;
//     return PARSE_OK;
// }

// static int parse_true(tinyjson_context* c, tinyjson_value* v){
//     EXPECT(c,'t');
//     if(c->json[0] !='r' || c->json[1]!='u' || c->json[2]!='e'){
//         return PARSE_INVALID_VALUE;
//     }
//     c->json += 3;
//     v->type =tinyjson_TRUE;
//     return PARSE_OK;
// }

// static int parse_false(tinyjson_context* c, tinyjson_value* v){
//     EXPECT(c,'f');
//     if(c->json[0] !='a' || c->json[1]!='l' || c->json[2]!='s' || c->json[3]!='e'){
//         return PARSE_INVALID_VALUE;
//     }
//     c->json += 4;
//     v->type =tinyjson_FALSE;
//     return PARSE_OK;
// }


static int parse_number(tinyjson_context* c, tinyjson_value* v){
    const char *p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else{
        if(!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    if(*p =='.'){
        p++;
        if(!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    if(*p == 'e' || *p=='E'){
        p++;
        if(*p == '+' || *p== '-') p++;
        if(!ISDIGIT(*p)) return PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    errno = 0;
    v->u.n = strtod(c->json,NULL);
    if(errno == ERANGE && (v->u.n == HUGE_VAL) || v->u.n == -HUGE_VAL){
        return PARSE_NUMBER_TOO_BIG;
    }
    v->type = tinyjson_NUMBER;
    c->json = p;
    return PARSE_OK;
}


/// @brief 解析字符串
/// @param c 
/// @param v 
/// @return 
static int parse_string(tinyjson_context* c, tinyjson_value* v){
    size_t head = c->top, len;
    const char *p;
    EXPECT(c, '\"');
    p = c->json;
    for(; ; ){
        char ch = *p++;
        switch(ch){
            case '\"':  //如果是 "
                len = c->top - head;
                set_string(v,(const char*)context_pop(c,len),len);
                c->json = p;
                return PARSE_OK;
            case '\\':
                switch(*p++){
                    case '\"': PUTC(c,'\"'); break;
                    case '\\': PUTC(c,'\\'); break;
                    case '/' : PUTC(c,'/'); break;
                    case 'b' : PUTC(c,'\b'); break;
                    case 'f' : PUTC(c,'\f'); break;
                    case 'n' : PUTC(c,'\n'); break;
                    case 'r' : PUTC(c,'\r'); break;
                    case 't' : PUTC(c,'\t'); break;
                    default:
                        c->top = head;
                        return PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0': //如果是空字符
                c->top = head;
                return PARSE_MISS_QUOTATION_MARK;
            default:
                if((unsigned char)ch < 0x20){
                    c->top = head;
                    return PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c,ch);
        }
    }
}


/// @brief 解析json value
/// @param c 
/// @param v 
/// @return 
static int parse_value(tinyjson_context* c, tinyjson_value* v){
    switch(*c->json){
        case 'n': 
            return parse_literal(c,v,"null",tinyjson_NULL);
        case 't':
            return parse_literal(c,v,"true",tinyjson_TRUE);
        case 'f':
            return parse_literal(c,v,"false",tinyjson_FALSE);
        case '"':
            return parse_string(c,v);
        case '\0':
            return PARSE_EXPECT_VALUE;
        default:
            return parse_number(c,v);
    }
}



/// @brief JSON语法 ： JSON-text = ws value ws
/// @param v 
/// @param json 
/// @return 
int parse(tinyjson_value *v, const char *json)
{
    tinyjson_context c;
    int ret;
    assert(v != NULL); //断言
    c.json = json; //方便后面传参数
    c.stack = NULL;
    c.size = c.top = 0;
    init(v);
    v->type = tinyjson_NULL;
    parse_whitespace(&c);
    if((ret=parse_value(&c,v)) == PARSE_OK){
        parse_whitespace(&c);
        if(*c.json != '\0'){
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack); //释放内存
    return ret;
}


/// @brief 释放内存
/// @param v 
void tinyjson_free(tinyjson_value *v){
    assert(v != NULL);
    if(v->type == tinyjson_STRING) 
        free(v->u.s.s);
    
    v->type = tinyjson_NULL;
}



/// @brief 获取v 数据类型type
/// @param v 
/// @return 
tinyjson_type get_type(const tinyjson_value *v){
    assert(v != NULL);
    return v->type;
}

/// @brief 获取数字
/// @param v 
/// @return 
double get_number(const tinyjson_value *v){
    assert(v != NULL && v->type == tinyjson_NUMBER);
    return v->u.n;
}

/// @brief 
/// @param v 
/// @param n 
void set_number(tinyjson_value *v,double n){
    tinyjson_free(v);
    v->u.n = n;
    v->type = tinyjson_NUMBER;
}

/// @brief 
/// @param v 
/// @return 
int get_boolean(const tinyjson_value *v){
    assert(v != NULL && (v->type == tinyjson_TRUE || v->type == tinyjson_FALSE));
    return v->type == tinyjson_TRUE;
}


/// @brief 设置布尔值
/// @param v 
/// @param b 
void set_boolean(tinyjson_value *v, int b){
    tinyjson_free(v);
    v->type = b ? tinyjson_TRUE : tinyjson_FALSE;
}


const char* get_string(const tinyjson_value * v){
    assert(v != NULL && v->type == tinyjson_STRING);
    return v->u.s.s;
}

size_t get_string_length(const tinyjson_value *v){
    assert(v != NULL && v->type == tinyjson_STRING);
    return v->u.s.len;
}


void set_string(tinyjson_value * v, const char * s, size_t len){
    assert(v != NULL && (s!=NULL || len == 0));
    tinyjson_free(v);
    v->u.s.s = (char*)malloc(len+1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = tinyjson_STRING;
}