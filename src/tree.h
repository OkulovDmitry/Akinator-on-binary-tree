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

#define TREE_DEBUG

typedef enum { GV_ARCS, GV_ORTHO } gv_mode_t;

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

typedef struct Node {
    int key;
    node_elem_t data;
    struct Node* left;
    struct Node* right;
    struct Node* head;
} Node;

typedef struct {
    Node* root;
    size_t size;
} Tree;

/* ---- создание и удаление ---- */
void tree_init(Tree* t);
void tree_destroy(Tree* t);

/* ---- вставка/удаление узла за O(n) ---- */
tree_err_t tree_insert(Tree* t, int key, node_elem_t data);
Node* tree_find(const Tree* t, int key);
tree_err_t tree_remove_node(Tree* t, Node* n);
tree_err_t tree_remove(Tree* t, int key);

/* ---- отладка ----*/
void log_init();

void tree_dump_hook(FILE* o, void* ctx);

void tree_dump(FILE* o, const Tree* t);

tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit);
tree_err_t tree_verify(const Tree* t);


// ---- Дополнительные функции ----
tree_err_t node_inorder(FILE* o, const Node* n); 
tree_err_t tree_inorder(FILE* o, const Tree* t); // вывод элементов дерева по возрастанию key
Node* tree_min(Node* n); // поиск минимального узла (самый левый)
Node* tree_max(Node* n); // поиск максималного узла (самый правый)
int tree_height(const Node* n); // нахождение высоты дерева

#endif