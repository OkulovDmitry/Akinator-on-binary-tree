#include "tree.h"

/* ---- Инициализация дерева ---- */
void tree_init(Tree* t) {
    ASSERT(t != NULL);
    t->root = NULL;
    t->size = 0;
    ASSERT(tree_verify(t) == TREE_OK);
}

static void destroy_node(Node* n) {
    if (!n) return;
    destroy_node(n->left);
    destroy_node(n->right);
    free(n);
}

/* ---- Освобождает все узлы - обход снизу вверх ---- */
void tree_destroy(Tree* t) {
    ASSERT(t != NULL);
    destroy_node(t->root);
    t->root = NULL;
    t->size = 0;
}

/* ---- Вставка узла ---- */
tree_err_t tree_insert(Tree* t, int key, node_elem_t data) {
    ASSERT(t != NULL);

    Node* parent = NULL;
    Node** pp    = &t->root; /* указатель на место куда писать */
    while (*pp) {
        parent = *pp; /* запоминание последнего пройденного*/
        if (key == parent->key) return TREE_DUPLICATE;
        pp = (key < parent->key) ? &parent->left : &parent->right;
    }

    Node* n = malloc(sizeof(*n));
    if (!n) return TREE_NO_MEM;

    *n  = (Node){.key = key, .data = data, .head = parent};
    *pp = n;
    t->size++;

    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

/* ---- Поиск узла ---- */
Node* tree_find(const Tree* t, int key) {
    ASSERT(t != NULL);
    Node* cur = t->root;
    while (cur && key != cur->key) {
        cur = (key < cur->key) ? cur->left : cur->right;
    }
    return cur;
}

/* заменяет поддерево u поддеревом v */
static void transplant(Tree* t, Node* u, Node* v) {
    if (!u->head)
        t->root = v; /* u был корнем дерева */
    else if (u == u->head->left)
        u->head->left = v;
    else
        u->head->right = v;

    if (v) v->head = u->head;
}

/* ---- Удаление узла ---- */
tree_err_t tree_remove_node(Tree* t, Node* n) {
    ASSERT(t != NULL);
    ASSERT(n != NULL);
    if (!n->left)
        transplant(t, n, n->right); /* нет детей или только правый */
    else if (!n->right)
        transplant(t, n, n->left);    /* только левый */
    else {                            /* 2 ребенка */
        Node* s = tree_min(n->right); /* преемник как минимум правого поддерева */

        if (s->head != n) {             /* преемник глебже n->right */
            transplant(t, s, s->right); /* вырезаем его и поднимаем правого ребенка */
            s->right       = n->right;  /* к преемнику цепляем правое поддерево */
            s->right->head = s;
        }
        transplant(t, n, s); /* преемник встает на место n */
        s->left       = n->left;
        s->left->head = s;
    }
    /* Кароче у s точно нет левого ребенка, тк он самый левый в правом поддереве. 
   Если s не прямой правый ребенок n, то мы его вырезаем из текущего места - transplant(t, s, s->right); - ну то есть пихаем его правую часть
   на его место. 
   Далее правому ребенку s отдаем все правод поддерево n. А хэдером для правого поддерева назначаем s.
   Ну и теперь можно независимо от случая ставить s на место n.
   */

    free(n);
    t->size--;
    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

/* Поиск узла по key и его удаление */
tree_err_t tree_remove(Tree* t, int key) {
    ASSERT(t != NULL);

    Node* n = tree_find(t, key);
    if (!n) return TREE_NOT_FOUND;

    return tree_remove_node(t, n);
}

static void dump_node(FILE* o, const Node* n, size_t* budget) {
    if (!n) {
        fputs("nil", o);
        return;
    }

    if (*budget == 0) { /* цикл или битый size */
        fputs("...", o);
        return;
    }
    (*budget)--;

    fputc('(', o);
    dump_node(o, n->left, budget);
    fputc(' ', o);
    dump_node(o, n->right, budget);
    fprintf(o, " %d)", n->key);
}

void tree_dump(FILE* o, const Tree* t) {
    ASSERT(o != NULL);
    ASSERT(t != NULL);

    size_t budget = t->size;
    fprintf(o, "tree(size = %zu): ", t->size);
    dump_node(o, t->root, &budget);
    fputc('\n', o);
}

void tree_dump_hook(FILE* o, void* ctx) {
    tree_dump(o, (const Tree*)ctx);
}

static tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit) {
    if (!n) return TREE_OK;
    if (++*count > limit) return TREE_CORRUPTED;
    if (lo && n->key <= *lo) return TREE_WRONG_KEY;
    if (hi && n->key >= *hi) return TREE_WRONG_KEY;

    if (n->left && n->left->head != n) return TREE_CORRUPTED;
    if (n->right && n->right->head != n) return TREE_CORRUPTED;

    tree_err_t err = verify_node(n->left, lo, &n->key, count, limit);
    if (err != TREE_OK) return err;
    return verify_node(n->right, &n->key, hi, count, limit);
}

static tree_err_t tree_verify(const Tree* t) {
    if (!t) return TREE_NULL_PTR;
    size_t count = 0;
    if (t->root && t->root->head != NULL) return TREE_CORRUPTED;
    tree_err_t tree_err = verify_node(t->root, NULL, NULL, &count, t->size);
    if (tree_err != TREE_OK) return tree_err;
    if (count != t->size) return TREE_CORRUPTED;
    return TREE_OK;
}

tree_err_t node_inorder(FILE* o, const Node* n) {
    ASSERT(o != NULL);
    ASSERT(n != NULL);

    if (n->left != NULL) node_inorder(o, n->left);
    fprintf(o, "key: %d\ndata: " NODE_ELEM_FMT "\n\n", n->key, n->data);
    if (n->right != NULL) node_inorder(o, n->right);

    return TREE_OK;
}

tree_err_t tree_inorder(FILE* o, const Tree* t) {
    ASSERT(o != NULL);
    ASSERT(t != NULL);

    if (!(t->root)) fprintf(o, "Empty tree");
    node_inorder(o, t->root);
    return TREE_OK;
}

Node* tree_min(Node* n) {
    ASSERT(n != NULL);
    while (n->left) n = n->left;
    return n;
}

Node* tree_max(Node* n) {
    ASSERT(n != NULL);
    while (n->right) n = n->right;
    return n;
}

int tree_height(const Node* n) {
    if (!n) return 0;
    int height_left = 0, height_right = 0;
    if (n->left)
        height_left = tree_height(n->left);
    if (n->right)
        height_right = tree_height(n->right);

    return ((height_left > height_right) ? height_left : height_right) + 1;
}