#ifndef TREE_H
#define TREE_H

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "my_assert.h"
#include "stack.h"

//typedef double node_elem_t;

#define TREE_SEP "=========================================================================================================="

#define TREE_DEBUG

#ifdef TREE_DEBUG
#    define TREE_PLACE_IN , const char *name, const char *function, const char *file, int line
#    define TREE_PLACE_OUT(l) , #l, __func__, __FILE__, __LINE__
#else
#    define TREE_PLACE_IN
#    define TREE_PLACE_OUT(l)
#endif

#define TREE_GV_MODE "ARCS"

typedef enum {
    TREE_OK = 0,  //
    // WARNING
    TREE_NULL_PTR      = 1,  //
    NODE_NULL_PTR      = 2,  //
    TREE_NOT_FOUND     = 3,  //
    TREE_NO_MEM        = 4,  //
    TREE_IO_ERROR      = 5,  //
    TREE_DOT_CANT_OPEN = 6,  //
    NULL_PTR           = 7,  //
    // ERROR
    TREE_DUPLICATE                     = 100,  //
    TREE_WRONG_KEY                     = 101,  //
    TREE_WRONG_SIZE                    = 102,  //
    TREE_WRONG_HEAD                    = 103,  //
    NODE_LEFT_BROKEN                   = 104,  //
    NODE_RIGHT_BROKEN                  = 105,  //
    TREE_CORRYPTED                     = 106,  //
    TREE_WRONG_NODE_TYPE               = 107,  //
    DOUBLE_QUOTE_MISSING               = 108,  //
    NILL_MISSING                       = 109,  //
    ERROR_STACK_PUSH_IN_RPN            = 110,  //
    ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND = 111,
    ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND  = 112,
} tree_err_t;

typedef enum {
    NON_TYPE    = 0,
    INT_TYPE    = 1,
    DOUBLE_TYPE = 2,
    STRING_TYPE = 3,
    CHAR_TYPE   = 4,
    NUMBER_OF_TYPES,
} node_type;

extern const char* printf_types[];

typedef union {
    int int_data;
    double double_data;
    char* string_data;
    char char_data;
} node_elem_t;

typedef struct Node {
    int key;
    int type;
    node_elem_t data;
    struct Node* left;
    struct Node* right;
    struct Node* head;
} Node;

typedef struct {
    Node* root;
    size_t size;
#ifdef TREE_DEBUG
    const char* name;
    const char* function;
    const char* file;
    int line;
#endif
} Tree;

/* ---- создание и удаление ---- */
void tree_init(Tree* t TREE_PLACE_IN);
#define TREE_INIT(t) tree_init((t)TREE_PLACE_OUT(t))
void tree_destroy(Tree* t);

/* ---- вставка/удаление узла за O(n) ---- */
tree_err_t tree_insert(Tree* t, int key, node_type type, node_elem_t data);
Node* tree_find(const Tree* t, int key);
tree_err_t tree_remove_node(Tree* t, Node* n);
tree_err_t tree_remove(Tree* t, int key);

/* ---- отладка ----*/
void tree_log_init();

const char* tree_strerror(tree_err_t err);

void tree_dump_hook(const char* tree, void* ctx);

void tree_dump(const char* name, const Tree* t);
tree_err_t tree_dump_graph_at(const char* base_name, const Tree* tree,
                              const char* call_file, int call_line, const char* call_func);
#define tree_dump_graph(base, tree) tree_dump_graph_at((base), (tree), __FILE__, __LINE__, __func__)

tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit);
tree_err_t tree_verify(const Tree* t);

// ---- Дополнительные функции ----
void tree_renumber(Tree* t);
tree_err_t node_inorder(FILE* o, const Node* n);
tree_err_t tree_inorder(FILE* o, const Tree* t);  // вывод элементов дерева по возрастанию key
Node* tree_min(Node* n);                          // поиск минимального узла (самый левый)
Node* tree_max(Node* n);                          // поиск максималного узла (самый правый)
int tree_height(const Node* n);                   // нахождение высоты дерева
int bfs(const Tree* t);                           // нахождение ширины дерева - максимальное число узлов на одном уровне

tree_err_t read_rpn_tree(Tree* t, const char* name);

#endif