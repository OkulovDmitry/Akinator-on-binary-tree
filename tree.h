#ifndef TREE_H
#define TREE_H

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "my_assert.h"

typedef double node_elem_t;

#define NODE_ELEM_FMT "%f"
#define NODE_POISON ((tree_elem_t)0xDEADBEEF)

#define TREE_SEP "=========================================================================================================="

typedef enum {
    TREE_OK        = 0,
    TREE_NULL_PTR  = 1,
    TREE_NO_MEM    = 2,
    TREE_DUPLICATE = 3,
    TREE_CORRUPTED = 4,
    TREE_WRONG_KEY = 5,
    NODE_NULL_PTR  = 6,
    TREE_NOT_FOUND = 7,
} tree_err_t;

typedef struct {
    int key;
    node_elem_t data;
    Node* left;
    Node* right;
    Node* head;
} Node;

typedef struct {
    Node* root;
    size_t size;
} Tree;

void tree_init(Tree* t);

static void destroy_node(Node* n);
void tree_destroy(Tree* t);

tree_err_t tree_insert(Tree* t, int key, node_elem_t data);
Node* tree_find(const Tree* t, int key);
static void transplant(Tree* t, Node* u, Node* v);
tree_err_t tree_remove_node(Tree* t, Node* n);
tree_err_t tree_remove(Tree* t, int key);

void tree_dump_hook(FILE* o, void* ctx);
static void dump_node(FILE* o, const Node* n, size_t* budget);
void tree_dump(FILE* o, const Tree* t);

static tree_err_t node_verify(Node* n, const int* lo, const int* hi, size_t* count, size_t limit);
static tree_err_t tree_verify(const Tree* t);

tree_err_t node_inorder(FILE* o, const Node* n);
tree_err_t tree_inorder(FILE* o, const Tree* t);
Node* tree_min(Node* n);
Node* tree_max(Node* n);
int tree_height(const Node* n);

#endif