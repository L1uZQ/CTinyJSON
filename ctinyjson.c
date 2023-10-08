#include"ctinyjson.h"
#include<assert.h>
#include<stdlib.h>

#define EXPECT(c, ch)   do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct{
    const char *json;
}tinyjson_context;

static void parse_whitespace(tinyjson_context *c){
    const char *p = c->json;
    while(*p==' ' || *p=='\t' || *p=='\n' || *p=='\r')
        p++;
    c->json = p;
}

static int parse_null(tinyjson_context* c, tinyjson_value* v){
    EXPECT(c,'n');
    if(c->json[0] !='u' || c->json[1]!='l' || c->json[2]!='l'){
        return PARSE_ROOT_NOT_SINGULAR;
    }
    c->json += 3;
    v->type =tinyjson_NULL;
    return PARSE_OK;
}

static int parse_value(tinyjson_context* c, tinyjson_value* v){
    switch(*c->json){
        case 'n':
            return parse_null(c,v);
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
    assert(v != NULL);
    c.json = json;
    v->type = tinyjson_NULL;
    parse_whitespace(&c);
    return parse_value(&c, v);
}

tinyjson_type get_type(const tinyjson_value *v){
    assert(v != NULL);
    return v->type;
}