#include "common.h"

struct config {
    char* key;
    char* value;
    struct config* next;
};
static struct config* conf = NULL;
static FILE* fp;

char* get_config(char* key)
{
    struct config* cur = conf;
    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0)
            return cur->value;
        cur = cur->next;
    }
    fprintf(stderr, "config '%s' does not exist\n", key);
    return NULL;
}

// note: a value with an existing key overrides the old one
static void add_config(char* key, char* value)
{
    struct config *new = malloc(sizeof(struct config));
    new->key = strdup(key);
    new->value = strdup(value);
    new->next = conf;
    conf = new;
}

static char c;
static char __next()
{
    if (1 != fread(&c, 1, 1, fp))
        c = 0;
    return c;
}

static bool parse_config()
{
#define MAX_STRLEN 100
    char key[MAX_STRLEN] = {0};
    char value[MAX_STRLEN] = {0};

#define next() ({if (__next() == 0) return false; c; })
#define next_maybe_eof() ({if (__next() == 0) return false; c; })

    next_maybe_eof();
beginline:
    if (c == ';' || c == '[') { // comment or section name, read until \n
        while (next() != '\n');
        goto beginline;
    }
    if (c == '\n') // continuous \n
        goto beginline;
readkey:
    if (isgraph(c)) {
        if (c == '=') {
            next();
            goto readvalue;
        }
        key[strlen(key)] = c;
    }
    next();
    goto readkey;
readvalue:
    if (isgraph(c) && c != '"') {
        if (c == '\n') {
            add_config(key, value);
            key[0] = value[0] = 0;
            next_maybe_eof();
            goto beginline;
        }
        value[strlen(value)] = c;
    }
    next();
    goto readvalue;
}

bool load_config()
{
    fp = fopen("config.ini", "r");
    return parse_config();
}
