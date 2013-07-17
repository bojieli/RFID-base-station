#include "common.h"

dict new_dict(void) {
    dict new = malloc(sizeof(dict_item));
    new->key = NULL;
    new->next = NULL;
    new->value = 0;
    return new; // this is a sentinel
}
int get(dict d, char* key) {
    dict curr = d->next;
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return 0; // not found, 0 for default
}
bool set(dict d, char* key, int value) {
    dict curr = d->next;
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            curr->value = value;
            return true;
        }
        curr = curr->next;
    }
    dict new = malloc(sizeof(dict_item));
    new->key = strdup(key);
    new->next = d->next;
    new->value = value;
    d->next = new;
    return true;
}
void __remove(dict_item* curr) {
    dict tobefree = curr->next;
    free(tobefree->key);
    curr->next = tobefree->next;
    free(tobefree);
}
bool del(dict d, char* key) {
    dict curr = d;
    while (curr->next != NULL) {
        if (strcmp(curr->next->key, key) == 0) {
            __remove(curr);
            return true;
        }
        curr = curr->next;
    }
    return false; // not found
}

