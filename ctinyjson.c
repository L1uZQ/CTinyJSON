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

/// @brief 读取4位16进制数据
/// @param p 字符串当前解析到的位置
/// @param u 传出参数，4位16进制数据
/// @return 解析成功返回解析后的p指针位置，失败返回NULL
static const char* parse_hex4(const char* p, unsigned* u) {
    /* \TODO */
    int i;
    *u=0;
    for(i=0; i<4; i++){
        char ch = *p++;
        *u <<= 4;
        if (ch>='0' && ch<='9') *u |= ch-'0';
        else if (ch>='A' && ch<='F') *u |= (ch-'A'+10);
        else if (ch>='a' && ch<='f') *u |= (ch-'a'+10);
        else return NULL;
    }
    return p;
}

/// @brief utf-8编码
/// @param c 
/// @param u 
static void encode_utf8(tinyjson_context* c, unsigned u) {
    /* \TODO */
    if(u<= 0x7F){ //可以用一个字节表示
        PUTC(c, u&0xFF);
    }
    else if(u<=0x7FF){ //用两个字节表示
        PUTC(c, 0xc0 | ((u>>6) & 0xFF));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    else if(u<=0xFFFF){ //用三个字节表示
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else{ //四个字节
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u & 0x3F));
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)



/// @brief 解析字符串
/// @param c 
/// @param v 
/// @return 
static int parse_string(tinyjson_context* c, tinyjson_value* v){
    size_t head = c->top, len;
    unsigned u, u2;
    const char *p;
    EXPECT(c, '\"');
    p = c->json;
    for(; ; ){
        char ch = *p++;
        switch(ch){
            case '\"':  //如果是 ",字符串结束
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
                    case 'u' :
                        if(!(p = parse_hex4(p,&u))){
                            // printf("testestetstet\n");
                            STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                        }
                        if(u >= 0xD800 && u<= 0xDBFF){
                            // printf("testestetstet\n");
                            if(*p++ != '\\')
                                STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                            if(*p++ != 'u') 
                                STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                            if(!(p = parse_hex4(p,&u2)))
                                STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
                            if(u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
                                //把高代理项和低代理项转换为真正的码点
                            u = (((u- 0xD800) << 10) | (u2-0xDC00)) + 0x10000;
                        }
                        encode_utf8(c,u);
                        break;
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

/*lept_parse_value() 会调用 lept_parse_array()，而 lept_parse_array() 
又会调用 lept_parse_value()，这是互相引用，所以必须要加入函数前向声明。*/
static int parse_value(tinyjson_context * c, tinyjson_value * v); //前向声明

static int parse_array(tinyjson_context * c, tinyjson_value * v){
    size_t i,size = 0;
    int ret;
    EXPECT(c,'[');
    //数组为空
    parse_whitespace(c);
    if(*c->json == ']'){
        c->json++;
        v->type=tinyjson_ARRAY;
        v->u.a.size=0;
        v->u.a.e = NULL;
        return PARSE_OK;
    }
    for(;;){
        tinyjson_value e;
        init(&e);
        if((ret = parse_value(c,&e)) != PARSE_OK)
            return ret;
        memcpy(context_push(c,sizeof(tinyjson_value)),&e,sizeof(tinyjson_value));
        size++;
        parse_whitespace(c);
        if(*c->json ==','){
            c->json++;
            parse_whitespace(c);
        }
        else if(*c->json==']'){
            c->json++;
            v->type = tinyjson_ARRAY;
            v->u.a.size = size;
            size *= sizeof(tinyjson_value);
            memcpy(v->u.a.e = (tinyjson_value*)malloc(size), context_pop(c,size),size);
            return PARSE_OK;
        }
        else  {
           ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
           break;
        }
    }
    
    for(int i=0; i<size; i++)
        tinyjson_free((tinyjson_value*)context_pop(c,sizeof(tinyjson_value)));
    return ret;
}

/// @brief 解析json对象
/// @param c 
/// @param v 
/// @return 
static int parse_object(tinyjson_context * c, tinyjson_value * v){
    size_t size;
    tinyjson_member m;
    int ret;
    EXPECT(c,'{');
    parse_whitespace(c);
    if(*c->json == '}'){
        c->json++;
        v->type = tinyjson_OBJECT;
        v->u.o.m=0;
        v->u.o.size=0;
        return PARSE_OK;
    }
    m.k=NULL;
    size=0;
    for(;;){
        init(&m.v);
        if((ret = parse_value(c,&m.v)) != PARSE_OK)
            break;
        memcpy(context_push(c,sizeof(tinyjson_member)),&m,sizeof(tinyjson_member));
        size++;
        m.k=NULL;
    }
    return ret;
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
        case '[':
            return parse_array(c,v);
        case '{':
            return parse_object(c,v);
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
    // v->type = tinyjson_NULL;
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
    switch(v->type){
        case tinyjson_STRING:
            free(v->u.s.s);
            break;
        case tinyjson_ARRAY:
            for(size_t i=0; i<v->u.a.size; i++){
                //数组内的元素通过递归调用释放
                tinyjson_free(&v->u.a.e[i]);
            }
            free(v->u.a.e);
            break;
        default:
            break;
    }
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

size_t get_array_size(const tinyjson_value * v){
    assert(v!=NULL && v->type == tinyjson_ARRAY);
    return v->u.a.size;
}

tinyjson_value* get_array_element(const tinyjson_value * v, size_t index){
    assert(v != NULL && v->type == tinyjson_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

size_t get_object_size(const tinyjson_value* v){
    assert(v != NULL && v->type == tinyjson_OBJECT);
    return v->u.o.size;
}

const char* get_object_key(const tinyjson_value* v, size_t index){
    assert(v != NULL && v->type == tinyjson_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t get_object_key_length(const tinyjson_value* v, size_t index){
    assert(v != NULL && v->type == tinyjson_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

tinyjson_value * get_object_value(const tinyjson_value* v, size_t index){
    assert(v != NULL && v->type == tinyjson_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}