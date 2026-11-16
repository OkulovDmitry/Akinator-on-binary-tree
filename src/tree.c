#include "tree.h"

#define GV_BG "#1B1F24"         // фон
#define GV_TEXT "#E6EDF3"       // текст
#define GV_USED_FILL "#2F5D8A"  // в data 
#define GV_USED_BORDER "#6CA6E0"
#define GV_FREE_FILL "#2B3036"  // свободная ячейка
#define GV_FREE_BORDER "#6E7681"
#define GV_HDR_FILL "#5e4592"  // ячейка 0
#define GV_HDR_BORDER "#B392F0"
#define GV_NEXT "#3FB950"        // next
#define GV_PREV "#F0883E"        // prev
#define GV_BOTH "#187a06"        // согласованная связь next + prev
#define GV_FREE_CHAIN "#8B949E"  // цепочка свободных
#define GV_FREE_PTR "#D29922"    // указатель free
#define GV_BAD "#F85149"         // рассогласование

const char* list_strerror(tree_err_t err) {
    switch (err) {
        case TREE_OK:
            return "ok";
        case TREE_NULL_PTR:
            return "null pointer";
        case TREE_NO_MEM:
            return "out_of_memory (nodes)";
        case TREE_DUPLICATE:
            return "duplicate of key in tree";
        case TREE_WRONG_SIZE:
            return "tree have wrong size";
        case TREE_WRONG_HEAD:
            return "root have a parent";
        case NODE_LEFT_BROKEN:
            return "parent of left child is not this node";
        case NODE_RIGHT_BROKEN:
            return "parent of right child is not this node";
        default:
            return "unknown error";
    }
}

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
tree_err_t tree_insert(Tree* t, int key, node_type type, node_elem_t data) {
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

    if (type >= NUMBER_OF_TYPES) return TREE_WRONG_TYPE;

    *n  = (Node){.key = key, .type = type, .data = data, .head = parent};
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

tree_err_t list_dump_graph(const Tree* tree, const char* base_name) {
    if (tree == NULL || base_name == NULL) return TREE_NULL_PTR;

    char dot[256], png[256];
    tree_err_t err;

    static int dump_id = 0;
    dump_id++;

    FILE* out = fopen(dot, "w");
    if (out == NULL) return TREE_IO_ERROR;

    FILE* log_file = fopen("log.html", "a");
    if (log_file == NULL) {
        fclose(out);
        return TREE_IO_ERROR;
    }

    const char* mode_str = GV_MODE;
    fprintf(log_file, "  <div class=\"meta\"><strong>DUMP #%d</strong> | Mode: <span class=\"meta-func\">%s</span> | File: <span class=\"meta-file\">%s</span></div>\n",
            dump_id, mode_str, png);
    fprintf(log_file, "  <table class=\"stats-table\">\n");
    fprintf(log_file,
            "    <tr>"
            "<td class=\"var-name\">Size:</td><td class=\"var-val\">%d</td>"
            "<td class=\"var-name\">Key of root:</td><td class=\"var-val\">%d</td>"
            "<td class=\"var-name\">Data of root:</td><td class=\"var-val\">%d</td>"
            "<td class=\"var-name\">Key of left child:</td><td class=\"var-val\">%d</td>"
            "<td class=\"var-name\">Key of right child:</td><td class=\"var-val\">%d</td>"
            "</tr>\n",
            tree->size, tree->root->key, tree->root->data, tree->root->left->key, tree->root->right->key);
    fprintf(log_file, "  </table>\n");

    // ============================ заголовок =========================
    fprintf(out, "digraph list {\n");
    const char* splines = (GV_ORTHO == "ORTHO") ? "ortho" : "true";
    double nodesep      = (GV_ORTHO == "ORTHO") ? 2.0 : 1.6;
    double ranksep      = (GV_ORTHO == "ORTHO") ? 2.5 : 1.5;
    fprintf(out, "  graph [dpi=110, bgcolor=\"" GV_BG
                 "\", splines=%s, pad=0.5,"
                 " nodesep=%.1f, ranksep=%.1f];\n",
            splines, nodesep, ranksep);
    fprintf(out, "  node  [shape=rectangle, style=\"rounded,filled\", fontcolor=\"" GV_TEXT
                 "\","
                 " fontsize=22, width=2.2, height=2.0, fixedsize=true, penwidth=3];\n");
    fprintf(out, "  edge  [penwidth=2, arrowsize=1.0];\n");

    //==================================================================================

    snprintf(dot, sizeof(dot), "./dot_of_dumps/%s_ortho.dot", base_name);
    snprintf(png, sizeof(png), "./png_dumps/%s_ortho.png", base_name);

    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "dot -Tpng \"%s\" -o \"%s\"", dot, png);
    if (system(cmd) != 0) {
        fprintf(log_file, "  <div style=\"color: #e06c75; font-weight: bold;\">[ERROR] Graphviz generation failed!</div>\n");
        fprintf(log_file, "</div>\n");  // Закрываем dump-card
        fclose(log_file);
        return TREE_CORRYPTED;
    }
}

void tree_dump_hook(FILE* o, void* ctx) {
    tree_dump(o, (const Tree*)ctx);
}

tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit) {
    if (!n) return TREE_OK;
    if (++*count > limit) return TREE_WRONG_SIZE;
    if (lo && n->key <= *lo) return TREE_WRONG_KEY;
    if (hi && n->key >= *hi) return TREE_WRONG_KEY;

    if (n->left && n->left->head != n) return NODE_LEFT_BROKEN;
    if (n->right && n->right->head != n) return NODE_RIGHT_BROKEN;

    tree_err_t err = verify_node(n->left, lo, &n->key, count, limit);
    if (err != TREE_OK) return err;
    return verify_node(n->right, &n->key, hi, count, limit);
}

tree_err_t tree_verify(const Tree* t) {
    if (!t) return TREE_NULL_PTR;
    size_t count = 0;
    if (t->root && t->root->head != NULL) return TREE_WRONG_SIZE;
    tree_err_t tree_err = verify_node(t->root, NULL, NULL, &count, t->size);
    if (tree_err != TREE_OK) return tree_err;
    if (count != t->size) return TREE_WRONG_HEAD;
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

void log_init() {
    FILE* log = fopen("log.html", "w");
    if (!log) return;

    /* 1 строка - говорит браузеру чт осейчас будет современный HTML5 и его нужно читать по актуальным правилам
       meta - это для текста на русском
       4 строка - задает текст, который будет написан на самой вкладке в браузере рядом с иконкой
       далее в стайле задается правила внешнего вида для тегов:
           выбираем тег боди и пишем условия: цвета окружения текста отступы и прочее
           далее следующий тег - натсройка внешнего вида главного заголовка страницы и самой карточки дампа
           и следующий тег - натсройка дампа самой картинки
           и далее настройка текста внутри карточки  - где и почему вызван дамп, таблички с переменными окраска и прочее
           и завершающий блок - чтобы картинка не ломала верстку и читалась на темном фоне
       */

    fprintf(log,
            "<!DOCTYPE html>\n"
            "<html>\n<head>\n"
            " <meta charset=\"UTF-8\">\n"
            " <title>Tree Lifecycle Log</title>\n"
            "<style>\n"
            "   body {\n"
            "       font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
            "       background-color: #13161A;\n"
            "       color: #C9D1D9;\n"
            "       padding: 30px;\n"
            "       margin: 0;\n"
            "   }\n"
            "   h1 {\n"
            "       color: #F0F6FC;\n"
            "       border-bottom: 2px solid #30363D;\n"
            "       padding-bottom: 10px;\n"
            "   }\n"
            "   .dump-card {\n"
            "       background-color: #1E232A;\n"
            "       border: 1px solid #30363D;\n"
            "       border-left: 5px solid #58A6FF;\n"
            "       border-radius: 6px;\n"
            "       padding: 20px;\n"
            "       margin-bottom: 25px;\n"
            "       box-shadow: 0 4px 6px rgba(0,0,0,0.3);\n"
            "   }\n"
            "   .meta {\n"
            "       font-size: 0.95em;\n"
            "       color: #85c1e9;\n"
            "       margin-bottom: 10px;\n"
            "   }\n"
            "   .meta-func { color: #dcdcaa; font-weight: bold; }\n"
            "   .meta-file { color: #ce9178; }\n"
            "   .stats-table {\n"
            "       border-collapse: collapse;\n"
            "       margin: 10px;\n"
            "   }\n"
            "   .stats-table td {\n"
            "       padding: 4px 15px 4px 0;\n"
            "   }\n"
            "   .var-name { color: #9cdcfe; font-weight: bold; }\n"
            "   .var-val { color: #b5cea8; }\n"
            "    .graph-container {\n"
            "        background-color: #1B1F24;\n"
            "        border: 1px solid #30363D;\n"
            "        padding: 15px;\n"
            "        border-radius: 4px;\n"
            "        margin-top: 15px;\n"
            "        display: inline-block;\n"
            "        max-width: 100%%;\n"
            "        overflow-x: auto;\n"
            "    }\n"
            "    img {\n"
            "        max-height: 600px;\n"
            "        display: block;\n"
            "    }\n"
            "       /* темный скроллбар */\n"
            "       ::-webkit-scrollbar {\n"
            "           width: 10px;\n"
            "           height: 10px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-track {\n"
            "           background: #16191E;\n"
            "           border-radius: 4px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-thumb {\n"
            "           background: #38414D;\n"
            "           border-radius: 4px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-thumb:hover {\n"
            "           background: #555555;\n"
            "    }\n"
            "    .error {\n"
            "        background-color: #3a1d1d;\n"
            "        border-left: 5px solid #f85149;\n"
            "        color: #ffb4ae;\n"
            "        padding: 10px 15px;\n"
            "        margin: 15px 0;\n"
            "        border-radius: 4px;\n"
            "   }\n"
            "</style>\n"
            "</head>\n<body>\n<h1>Tree Execution History</h1>\n");
    fclose(log);
}

static void html_escape(FILE* f, const char* s) {
    for (; *s; s++) {
        switch (*s) {
            case '&':
                fputs("&amp;", f);
                break;
            case '<':
                fputs("&lt;", f);
                break;
            case '>':
                fputs("&gt;", f);
                break;
            default:
                fputc(*s, f);
        }
    }
}