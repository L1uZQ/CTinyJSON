#include"ctinyjson.h"
#include<assert.h>
#include<stdlib.h>

#define EXPECT(c, ch)   do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct{
    const char *json;
}tinyjson_context;

// 过掉空格
static void parse_whitespace(tinyjson_context *c){
    const char *p = c->json;
    while(*p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
        p++;
    c->json = p;
}

static int parse_null(tinyjson_context* c, tinyjson_value* v){
    EXPECT(c,'n');
    if(c->json[0] !='u' || c->json[1]!='l' || c->json[2]!='l'){
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type =tinyjson_NULL;
    return PARSE_OK;
}

static int parse_true(tinyjson_context* c, tinyjson_value* v){
    EXPECT(c,'t');
    if(c->json[0] !='r' || c->json[1]!='u' || c->json[2]!='e'){
        return PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type =tinyjson_TRUE;
    return PARSE_OK;
}

static int parse_false(tinyjson_context* c, tinyjson_value* v){
    EXPECT(c,'f');
    if(c->json[0] !='a' || c->json[1]!='l' || c->json[2]!='s' || c->json[3]!='e'){
        return PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type =tinyjson_FALSE;
    return PARSE_OK;
}

static int parse_value(tinyjson_context* c, tinyjson_value* v){
    switch(*c->json){
        case 'n': 
            return parse_null(c,v);
        case 't':
            return parse_true(c,v);
        case 'f':
            return parse_false(c,v);
        case '\0':
            return PARSE_EXPECT_VALUE;
        default:
            return PARSE_INVALID_VALUE;
    }
}


// JSON语法 ： JSON-text = ws value ws
int parse(tinyjson_value *v, const char *json)
{
    tinyjson_context c;
    int ret;
    assert(v != NULL); //断言
    c.json = json; //方便后面传参数
    v->type = tinyjson_NULL;
    parse_whitespace(&c);
    if((ret=parse_value(&c,v)) == PARSE_OK){
        parse_whitespace(&c);
        if(*c.json != '\0'){
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

tinyjson_type get_type(const tinyjson_value *v){
    assert(v != NULL);
    return v->type;
}