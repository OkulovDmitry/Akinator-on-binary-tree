#include <stdio.h>

#include "tree.h"

/* VERIFY работает и в release, в отличие от ASSERT */
static void insert_ok(Tree* t, int key) {
    node_elem_t data;
    data.int_data = key * 10;
    tree_err_t e = tree_insert(t, key, INT_TYPE, data);
    VERIFY_MSG(e == TREE_OK, "insert %d failed, code=%d", key, e);
}

static void remove_ok(Tree* t, int key, char* name) {
    tree_err_t e = tree_remove(t, key);
    VERIFY_MSG(e == TREE_OK, "remove %d failed, code=%d", key, e);
    VERIFY_MSG(tree_find(t, key) == NULL, "key %d still in tree", key);
    printf("removed %-3d -> ", key);
    tree_dump_graph(name, t);
}

int main(void) {
    tree_log_init();

    Tree t;
    tree_init(&t);
    assert_set_hook(tree_dump_hook, &t); /* любой упавший ассерт сбросит дерево в лог */

    /*          50
              /    \
            30      70
           /  \    /  \
         20   40  60   80
                    \
                    65                                    */
    int keys[] = {22, 23, 50, 30, 70, 20, 40, 60, 80, 65, 1, 2, 3, 4, 5, 6, 7, 12, 33, 32, 56, 48, 43, 34, 36, 21, 24, 28, 90, 101, 31};
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++)
        insert_ok(&t, keys[i]);

    printf("after inserts: ");
    tree_dump_graph("stdout", &t);

    // ---- расскмоентить для проверки ассертов ----
    //t.size = 7;

    /* размер, высота, поиск */
    //VERIFY(t.size == 8);
    VERIFY(tree_height(t.root) == 4);
    VERIFY(tree_find(&t, 65) != NULL);
    VERIFY(tree_find(&t, 99) == NULL);

    /* дубликат и отсутствующий ключ это коды ошибок, а не падение */
    //VERIFY(tree_insert(&t, 40, 0) == TREE_DUPLICATE);
    //VERIFY(tree_remove(&t, 99) == TREE_NOT_FOUND);
    //VERIFY(t.size == 8);

    /* min / max */
    VERIFY(tree_min(t.root)->key == 20);
    VERIFY(tree_max(t.root)->key == 80);

    /* обход по возрастанию */
    printf("\ninorder:\n");
    tree_inorder(stdout, &t);

    t.root->left->head = t.root->right;

    /* удаления: каждый шаг ветка remove */
    remove_ok(&t, 20, "dump1"); /* лист */
    remove_ok(&t, 30, "dump2"); /* один ребёнок (остался только 40) */
    remove_ok(&t, 50, "dump3"); /* корень, два ребёнка, преемник 60 глубже n->right */
    remove_ok(&t, 60, "dump4"); /* корень, два ребёнка, преемник 65 глубже n->right */
    remove_ok(&t, 70, "dump5"); /* один ребёнок (80) */
    remove_ok(&t, 65, "dump6"); /* корень, два ребёнка, преемник 80 = n->right */

    //VERIFY(t.size == 2);

    /* пустое дерево после удаления всего */
    remove_ok(&t, 80, "dump7");
    remove_ok(&t, 40, "dump8");
    VERIFY(t.size == 0 && t.root == NULL);

    /* повторное использование после опустошения */
    insert_ok(&t, 1);
    //VERIFY(t.size == 1);

    tree_destroy(&t);
    VERIFY(t.root == NULL && t.size == 0);

    puts("\nALL TESTS PASSED");

    system("start chrome \"%CD%\\log.html\"");

    return 0;
}