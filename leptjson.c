#include "leptjson.h"

#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>  /* memcpy... */
#include <ctype.h>   /* isdigit() */
#include <math.h>    /* HUGE_VAL */
#include <errno.h>   /* errno */
#include <stdio.h>

typedef struct {
    const char* json;
} lept_context;

/* parse */

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
  	size_t i;

  	assert(*c->json == literal[0]);

  	for(i = 0; literal[i]; i ++) {
  		if(c->json[i] != literal[i]) {
  			return LEPT_PARSE_INVALID_VALUE;
  		}
  	}
    c->json += i;
    v->type = type;

    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* cur_phase = c->json;
    char* next_phase = NULL;

    assert(*cur_phase == '0' || *cur_phase == '1' || *cur_phase == '2' ||
        *cur_phase == '3' || *cur_phase == '4' || *cur_phase == '5' ||
        *cur_phase == '6' || *cur_phase == '7' || *cur_phase == '8' ||
        *cur_phase == '9' || *cur_phase == '-');

    if(*cur_phase == '0') {
    	/* avoid "00.." and hex number(strtod supports "00.." and hex number) */
        if(cur_phase[1] != '\0' && (cur_phase[1] == '0' || cur_phase[1] == 'x')) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    if(*cur_phase != '-') {
        /* has frac */
        if(cur_phase[1] != '\0' && cur_phase[1] == '.') {
        	/* avoid strings like "1.", "2.xas"... */
            if(cur_phase[2] == '\0' ||
              (cur_phase[2] && !isdigit(cur_phase[2]))) {
                return LEPT_PARSE_INVALID_VALUE;
            }
        }
    }

    /** strtod() succeeded,
     *    if and only if the string starts with a (hex or decimal) number
     */
    v->n = strtod(cur_phase, &next_phase);
    if(cur_phase == next_phase) {
        /* not a number */
        return LEPT_PARSE_INVALID_VALUE;
    } else if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL )) {
        /* out of range */
        errno = 0;
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json = next_phase;
    v->type = LEPT_NUMBER;

    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch(*c->json) {
        case '0': case '1': case '2': case '3': case '4':
        case '6': case '7': case '8': case '9': case '-':
            return lept_parse_number(c, v);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;

    assert(v != NULL);

    /* initialize */
    c.json = json;
    v->type = LEPT_NULL;
    ret = LEPT_PARSE_INVALID_VALUE;

    /* parse json */
    lept_parse_whitespace(&c);
    ret = lept_parse_value(&c, v);
    lept_parse_whitespace(&c);

    /* check end */
    if(ret == LEPT_PARSE_OK && *(c.json) != '\0') {
        ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
    }

    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

/* free */

void lept_free(lept_value* v) {
    assert(v != NULL);
    if(v->type == LEPT_STRING && v->len > 0) {
        free(v->s);
        v->len = 0;
    }
    /* to avoid double free */
    v->type = LEPT_NULL;
}

/* string */

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len != 0));
    lept_free(v);
    v->s = (char*)malloc((len + 1) * sizeof(char));
    memcpy(v->s, s, len);
    v->s[len] = '\0';
    v->len = len;
    v->type = LEPT_STRING;
}

/* boolean */

int lept_get_boolean(const lept_value* v) {
    // TODO
    return 0;
}

void lept_set_boolean(lept_value* v, int b) {
    // TODO
}

/* number */

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}

void lept_set_number(lept_value* v, double d) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    v->n = d;
}
