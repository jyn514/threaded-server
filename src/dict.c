/* Dictionary implementation */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dict.h"

#define INITIAL_HASH_SIZE 8
#define MAX_LOAD_FACTOR 2
#define SCALE_FACTOR 2

typedef struct dr {
    char *key, *value;
    struct dr *next;
} DICT_REC, *DR;

struct dict {
    int h_size, num_items;
    DR *hash_tab;
};

static bool insert_or_update(DICT, DR new_item);
static int hash(const char *key, int size);
static void insert_at_front(DR *list, DR new_item);
static DR remove_from_front(DR *list);
static void resize(DICT, int size);

DICT dict_init(void) {
    DICT d = malloc(sizeof(struct dict));
    d->h_size = 0;
    d->hash_tab = NULL;
    resize(d, INITIAL_HASH_SIZE);
    d->num_items = 0;
    return d;
}

// returns whether key already exists
bool dict_put(DICT dict, char *const key, char *const val) {
    DR p = (DR) malloc(sizeof(DICT_REC));
    p->key = key;
    p->value = val;
    return insert_or_update(dict, p);
}

char *dict_get(DICT dict, const char *key) {
    int index = hash(key, dict->h_size);
    DR p = dict->hash_tab[index];
    while (p != NULL && strcmp(key, p->key))
        p = p->next;
    return p == NULL ? NULL : p->value;
}

void dict_free(DICT dict) {
    for (int i = 0; i < dict->h_size; ++i) {
        DR current = dict->hash_tab[i];
        while (current != NULL) {
            DR temp = current;
            current = current->next;
            free(temp->value);
            free(temp->key);
            free(temp);
        }
    }
    free(dict->hash_tab);
    free(dict);
}

/* Local routines */

/* Returns whether key exists */
static bool insert_or_update(DICT dict, const DR new_item) {
    const char *key = new_item->key;
    int index = hash(key, dict->h_size);
    DR p = dict->hash_tab[index], *prev = dict->hash_tab + index;

    while (p != NULL && strcmp(key, p->key)) {
        prev = &p->next;
        p = p->next;
    }

    if (p == NULL) {
            /* Insertion */
        insert_at_front(prev, new_item);
        if (dict->num_items++ > MAX_LOAD_FACTOR*dict->h_size) {
            resize(dict, SCALE_FACTOR*dict->h_size);
        }
        return false;
    } else {
            /* Update */
        DR previous = remove_from_front(prev);
        free(previous->value);
        free(previous->key);
        free(previous);
        insert_at_front(prev, new_item);
        return true;
    }
}

static int hash(const char *key, const int size) {
    int sum = 0;
    for (; *key; key++)
        sum = (37*sum + *key) % size;
    return sum;
}

static void insert_at_front(DR *list, DR new_item) {
    new_item->next = *list;
    *list = new_item;
}

static DR remove_from_front(DR *list) {
    DR ret = *list;
    *list = (*list)->next;
    return ret;
}

static void resize(DICT dict, int size) {
    DR *temp = dict->hash_tab;
    int temp_size = dict->h_size;

    dict->h_size = size;
    dict->hash_tab = (DR *) calloc(size, sizeof(DR));

    // This only occurs on the initial sizing, with empty dictionary
    if (temp == NULL)
        return;

    for (int i = 0; i < temp_size; i++)
        while (temp[i] != NULL) {
            int index = hash(temp[i]->key, size);
            insert_at_front(dict->hash_tab+index, remove_from_front(temp+i));
        }
    free(temp);
}
