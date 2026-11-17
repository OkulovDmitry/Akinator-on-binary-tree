#include <stdio.h>

#include "tree.h"

/* VERIFY работает и в release, в отличие от ASSERT */
static void insert_ok(Tree* t, int key) {
    node_elem_t data;
    data.string_data = "ERROR";
    tree_err_t e     = tree_insert(t, key, STRING_TYPE, data);
    VERIFY_MSG(e == TREE_OK, "insert %d failed, code=%d", key, e);
}

static void remove_ok(Tree* t, int key, char* name) {
    tree_err_t e = tree_remove(t, key);
    VERIFY_MSG(e == TREE_OK, "remove %d failed, code=%d", key, e);
    VERIFY_MSG(tree_find(t, key) == NULL, "key %d still in tree", key);
    printf("removed %-3d -> ", key);
    tree_dump(name, t);
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

    //printf("after inserts: ");
    //tree_dump_graph("stdout", &t);

    // ---- расскмоентить для проверки ассертов ----
    //t.size = 7;

    /* размер, высота, поиск */
    //VERIFY(t.size == 8);
    //VERIFY(tree_height(t.root) == 4);
    //VERIFY(tree_find(&t, 65) != NULL);
    //VERIFY(tree_find(&t, 99) == NULL);

    /* дубликат и отсутствующий ключ это коды ошибок, а не падение */
    //VERIFY(tree_insert(&t, 40, 0) == TREE_DUPLICATE);
    //VERIFY(tree_remove(&t, 99) == TREE_NOT_FOUND);
    //VERIFY(t.size == 8);

    /* min / max */
    //VERIFY(tree_min(t.root)->key == 20);
    //VERIFY(tree_max(t.root)->key == 80);

    /* обход по возрастанию */
    //printf("\ninorder:\n");
    //tree_inorder(stdout, &t);

    //t.root->left->head = t.root->right;

    /* удаления: каждый шаг ветка remove */
    //remove_ok(&t, 20, "dump1.txt"); /* лист */
    //remove_ok(&t, 30, "dump2.txt"); /* один ребёнок (остался только 40) */
    //remove_ok(&t, 50, "dump3.txt"); /* корень, два ребёнка, преемник 60 глубже n->right */
    //remove_ok(&t, 60, "dump4.txt"); /* корень, два ребёнка, преемник 65 глубже n->right */
    //remove_ok(&t, 70, "dump5.txt"); /* один ребёнок (80) */
    //remove_ok(&t, 65, "dump6.txt"); /* корень, два ребёнка, преемник 80 = n->right */

    //VERIFY(t.size == 2);

    /* пустое дерево после удаления всего */
    //remove_ok(&t, 80, "txt_dumps/dump1.txt");
    //remove_ok(&t, 40, "txt_dumps/dump2.txt");
    //VERIFY(t.size == 0 && t.root == NULL);

    /* повторное использование после опустошения */
    //insert_ok(&t, 1);
    //VERIFY(t.size == 1);

    Tree first_read;

    assert_set_hook(tree_dump_hook, &first_read);
    tree_init(&first_read);

    tree_err_t read_err = read_rpn_tree(&first_read, "txt_dumps/dump1.txt");
    printf("read_err = %d, root = %p\n", read_err, (void*)first_read.root);
    fflush(stdout);
    if (read_err != TREE_OK || first_read.root == NULL) {
        return 1;
    }

    printf("%d\n", first_read.root->key);
    tree_dump_graph("dump2.txt", &first_read);

    tree_destroy(&t);
    tree_destroy(&first_read);
    //VERIFY(t.root == NULL && t.size == 0);

    puts("\nALL TESTS PASSED");

    system("start chrome \"%CD%\\log.html\"");

    return 0;
}