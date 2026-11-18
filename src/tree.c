#include "tree.h"

#define GV_BG "#1B1F24"         // фон
#define GV_TEXT "#E6EDF3"       // текст
#define GV_USED_FILL "#2F5D8A"  // в data
#define GV_USED_BORDER "#6CA6E0"
#define GV_FREE_FILL "#2B3036"  // свободная ячейка
#define GV_FREE_BORDER "#6E7681"
#define GV_HDR_FILL "#5e4592"  // корень
#define GV_HDR_BORDER "#B392F0"
#define GV_RIGHT "#F0883E"       // правый ребенок
#define GV_LEFT "#187a06"        // левый ребенок
#define GV_FREE_CHAIN "#8B949E"  // non_type
#define GV_FREE_PTR "#D29922"    // указатель  на non_type
#define GV_BAD "#F85149"         // рассогласование

#define MAX_NODE_LEN 500
#define HEAD_KEY 100
#define MAX_OF_NODES 1000

const char* printf_types[NUMBER_OF_TYPES] = {"%s", "%d", "%lf", "%s", "%c"};

const char* tree_strerror(tree_err_t err) {
    switch (err) {
        case TREE_OK:
            return "ok";
        case TREE_NULL_PTR:
            return "null tree pointer\n";
        case TREE_NO_MEM:
            return "out_of_memory (nodes)\n";
        case TREE_DUPLICATE:
            return "duplicate of key in tree\n";
        case TREE_WRONG_SIZE:
            return "tree have wrong size\n";
        case TREE_WRONG_HEAD:
            return "root have a parent\n";
        case NODE_LEFT_BROKEN:
            return "parent of left child is not this node\n";
        case NODE_RIGHT_BROKEN:
            return "parent of right child is not this node\n";
        case NODE_NULL_PTR:
            return "null node pointer\n";
        case TREE_NOT_FOUND:
            return "tree not found\n";
        case TREE_IO_ERROR:
            return "file processing failed\n";
        case TREE_WRONG_KEY:
            return "key is too big or small for his subtree\n";
        case TREE_DOT_CANT_OPEN:
            return "error running dot from the command line\n";
        case TREE_CORRYPTED:
            return "tree is not valid\n";
        case TREE_WRONG_NODE_TYPE:
            return "wrong type of data in node\n";
        case NULL_PTR:
            return "null pointer in function\n";
        case DOUBLE_QUOTE_MISSING:
            return "a double quote is missing in the file\n";
        case NILL_MISSING:
            return "expected nil, but got another word\n";
        case ERROR_STACK_PUSH_IN_RPN:
            return "error in stack_push in rpn\n";
        case ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND:
            return "expected a right child on the stack while inserting a new node into the tree, but none was found";
        case ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND:
            return "expected a left child on the stack while inserting a new node into the tree, but none was found";
        default:
            return "unknown error\n";
    }
}

/* ---- Инициализация дерева ---- */
void tree_init(Tree* t TREE_PLACE_IN) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    t->root = NULL;
    t->size = 0;

#ifdef TREE_DEBUG
    t->name     = name;
    t->function = function;
    t->file     = file;
    t->line     = line;
#endif

    tree_err_t err = tree_verify(t);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));
}

/* ---- Разрушение узла ---- */
static void destroy_node(Node* n) {
    if (!n) return;
    destroy_node(n->left);
    destroy_node(n->right);
    if (n->type == STRING_TYPE) free(n->data.string_data);
    free(n);
}

/* ---- Освобождает все узлы - обход снизу вверх ---- */
void tree_destroy(Tree* t) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    destroy_node(t->root);
    t->root = NULL;
    t->size = 0;
}

/* ---- Вставка узла ---- */
tree_err_t tree_insert(Tree* t, int key, node_type type, node_elem_t data) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    if (type >= NUMBER_OF_TYPES) return TREE_WRONG_NODE_TYPE;

    Node* parent = NULL;
    Node** pp    = &t->root;
    while (*pp) {
        parent = *pp;
        if (key == parent->key) return TREE_DUPLICATE;
        pp = (key < parent->key) ? &parent->left : &parent->right;
    }

    Node* n = malloc(sizeof(*n));
    if (!n) return TREE_NO_MEM;

    if (type == STRING_TYPE) {
        data.string_data = strdup(data.string_data ? data.string_data : "POISON");
        if (!data.string_data) {
            free(n);
            return TREE_NO_MEM;
        }
    }

    if (type == NON_TYPE) {
        data.string_data = NULL;
    }

    *n  = (Node){.key = key, .type = type, .data = data, .head = parent};
    *pp = n;
    t->size++;

    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

/* ---- Поиск узла ---- */
Node* tree_find(const Tree* t, int key) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

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
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));
    ASSERT_MSG(n != NULL, tree_strerror(NODE_NULL_PTR));
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

    if (n->type == STRING_TYPE) free(n->data.string_data);
    free(n);
    t->size--;
    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

/* Поиск узла по key и его удаление */
tree_err_t tree_remove(Tree* t, int key) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    Node* n = tree_find(t, key);
    if (!n) return TREE_NOT_FOUND;

    return tree_remove_node(t, n);
}

static void print_data(FILE* o, const Node* n) {
    switch (n->type) {
        case INT_TYPE:
            fprintf(o, "%d", n->data.int_data);
            break;
        case DOUBLE_TYPE:
            fprintf(o, "%lf", n->data.double_data);
            break;
        case STRING_TYPE:
            fprintf(o, "%s", n->data.string_data ? n->data.string_data : "(null)");
            break;
        case CHAR_TYPE:
            fprintf(o, "%c", n->data.char_data);
            break;
        default:
            fprintf(o, "POISON");
            break;
    }
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
    fprintf(o, " \"");
    print_data(o, n);
    fprintf(o, "\"");
    fprintf(o, ")");
}

void tree_dump(const char* name, const Tree* t) {
    ASSERT_MSG(name != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    FILE* o;
    if (strcmp(name, "stderr") == 0) {
        o = stderr;
    } else if (strcmp(name, "stdout") == 0) {
        o = stdout;
    } else {
        o = fopen(name, "w");
        ASSERT_MSG(o != NULL, tree_strerror(NULL_PTR));
    }

    size_t budget = t->size;
    //fprintf(o, "tree(size = %zu): ", t->size);
    dump_node(o, t->root, &budget);
    fputc('\n', o);

    if (o != stderr && o != stdout) fclose(o);
}

tree_err_t node_take(FILE* in, char* word, int* num_open, int* num_close, int* is_nil) {
    ASSERT_MSG(in != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(word != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_open != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_close != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(is_nil != NULL, tree_strerror(NULL_PTR));

    int c = fgetc(in);

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    while (c == ')') {
        (*num_close)++;
        c = fgetc(in);
    }

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    while (c == '(') {
        c = fgetc(in);
        (*num_open)++;
    }

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    int len = 0;
    *is_nil = 0;
    if (c == '"') {
        c = fgetc(in);
        while (c != '"' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            c           = fgetc(in);
        }
        if (c != '"') return DOUBLE_QUOTE_MISSING;
        word[len] = '\0';
    } else {
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            c           = fgetc(in);
        }
        word[len] = '\0';
        if (strcmp("nil", word) != 0) return NILL_MISSING;

        *is_nil = 1;

        if (c != EOF) ungetc(c, in);
    }

    return TREE_OK;
}

tree_err_t node_add_from_file(FILE* in, my_stack_t* nodes, int* num_open, int* num_close) {
    ASSERT_MSG(in != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(nodes != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_open != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_close != NULL, tree_strerror(NULL_PTR));

    char word[MAX_NODE_LEN];
    int is_nil;
    tree_err_t err = node_take(in, word, num_open, num_close, &is_nil);
    if (err != TREE_OK) return TREE_IO_ERROR;
    if (is_nil) {
        if (stack_push(nodes, NULL) != STACK_OK) return ERROR_STACK_PUSH_IN_RPN;
        return node_add_from_file(in, nodes, num_open, num_close);
    }

    Elem_t right_child;
    Elem_t left_child;
    if (stack_pop(nodes, &right_child) != STACK_OK) return ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND;
    if (stack_pop(nodes, &left_child) != STACK_OK) return ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND;

    Node* new_node = malloc(sizeof(Node));
    if (!new_node) return TREE_NO_MEM;
    new_node->left             = left_child;
    new_node->right            = right_child;
    new_node->head             = NULL;
    new_node->key              = 0;
    new_node->type             = STRING_TYPE;
    new_node->data.string_data = strdup(word);
    if (right_child) right_child->head = new_node;
    if (left_child) left_child->head = new_node;

    stack_push(nodes, new_node);
    int c = fgetc(in);
    if (c != ')') {
        printf("Expected ')', got '%c' (ASCII %d) after word '%s'\n", c, c, word);
        return TREE_CORRYPTED;
    }
    (*num_close)++;

    c = fgetc(in);
    while (c == ')' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        if (c == ')') (*num_close)++;
        c = fgetc(in);
    }

    if (c == EOF) return TREE_OK;
    ungetc(c, in);
    return node_add_from_file(in, nodes, num_open, num_close);
}

tree_err_t read_rpn_tree(Tree* t, const char* name) {
    ASSERT(name != NULL);
    FILE* in = fopen(name, "r");
    ASSERT(in != NULL);

    int c = fgetc(in);
    if (c != '(') {
        printf("Expected '(', got '%c' (ASCII %d) in begin of file %s'\n", c, c, name);
        return TREE_CORRYPTED;
    }

    int num_open  = 1;
    int num_close = 0;

    my_stack_t nodes;
    err_t stack_err = stack_ctor(&nodes, MAX_NODE_LEN);
    if (stack_err != STACK_OK) return TREE_CORRYPTED;

    tree_err_t err = node_add_from_file(in, &nodes, &num_open, &num_close);
    if (err != TREE_OK) return err;

    stack_err = stack_pop(&nodes, &(t->root));
    if (stack_err != STACK_OK) return TREE_CORRYPTED;

    t->size = num_open;
    fclose(in);

    tree_renumber(t);
    tree_err_t verify_err = tree_verify(t);
    ASSERT_MSG(verify_err == TREE_OK, strerror(verify_err));

    return TREE_OK;
}

/*
Использованные элементы графвиза:
========
    fprintf(out, "digraph list {\n"); - заголовок графа
digraph - значит граф ориентированный
list - его имя
========
    graph [dpi=100, bgcolor="#2F353B", splines=ortho, nodesep=0.6, ranksep=1.2]; - настройка всего графа
dpi - расширение картинки
bgcolor - цвет фона
splines=ortho - стрелки под прямым углом
nodesep=0.6 - расстояние меж узлами в одном ряду
ranksep - расстояние между рядами
=======
node  [shape=rectangle, style=\"rounded,filled\", fillcolor=\"#C5D0E6\","
            " color=\"#C5D0E6\", fontsize=20, width=1.6, height=2.0, fixedsize=true, penwidth=3]; - настройки для всех узлов по умолчанию
shape=rectangle - узел - прямоугольник со скругенными концами и заливкой
fillcolor - цвет заливки
color - цвет рамки
fixedsize=true - размер строго width * height (в дюймах, текст не растягивается)
penwidth - толщина рамки
=======
    edge  [penwidth=2, arrowsize=1.0]; - толщина стрелок и размер наконечника
=======
Переходим к ячейкам:
    fprintf(out, "  cell0 [color=red, label=\"header\\n\\nhead: %d\\ntail: %d\\nfree: %d\"];\n",
cell0 - объявление узла и его имя
в [] атрибуты:
color = red - перекрывает начальную рамку
label - это тест внутри
=======
    fprintf(out, "  { rank=same;");
    for (...) fprintf(out, " cell%d;", i);
    fprintf(out, " }\n");
rank=same заставляет Graphviz поставить перечисленные узлы на один горизонтальный уровень
=======
cell%d -> cell%d [style=invis, weight=100];
невидимые стрелки 0->1->2->... с большим весом. В итоге графвиз мтремится сделать эти невидимые стрелки с большим 
весом короткими и прямыми, поэтому ячейки выстраиваются не как добно графвизу, а слева направо в порядке индексов
=======
if (pv == -1) {... цепочка белых свободных ...}
для свободной ячейки рисуется только белая стрелка next
=======
if (list->prev[nx] == i) зелёная  else красная, penwidth=4
Это проверка согласованности в картинке - если next ячейки i указывает на nx то prev[nx] обязан быть i -
 - если нет то стрелка красная и толстая
 Для prev то же самое в обратную - оранжевая или красная
*/

// ================================== дамп ячейки в dot ==================================
tree_err_t node_dump_graph(const Node* n, int* qur_num, FILE* out) {
    if (n == NULL) return TREE_OK;
    if (*qur_num > 4 * MAX_OF_NODES) return TREE_CORRYPTED;

    if (*qur_num == 0)  // корень
    {
        fprintf(out, "  cell%d [fillcolor=\"" GV_HDR_FILL "\", color=\"" GV_HDR_BORDER
                     "\","
                     " label=\"ROOT\\n\\key: %d",
                *qur_num, n->key);
        fprintf(out, "\\ndata: ");
        print_data(out, n);

    } else {
        int is_free         = (n->type == NON_TYPE);
        int number_of_child = (n->left == NULL && n->right == NULL) ? 0 : ((n->right != NULL && n->left != NULL) ? 2 : 1);
        fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\"%s, label=\"key: %d[%d]\\n\\n",
                *qur_num,
                is_free ? GV_FREE_FILL : GV_USED_FILL,
                is_free ? GV_FREE_BORDER : GV_USED_BORDER,
                is_free ? ", style=\"rounded,filled,dashed\"" : "",
                n->key, number_of_child);
        print_data(out, n);
    }

    if (n->head != NULL)
        fprintf(out, "\\nhead: %d", n->head->key);
    else
        fprintf(out, "\\nhead: NO_HEAD");

    if (n->left != NULL)
        fprintf(out, "\\nleft_child: %d", n->left->key);
    else
        fprintf(out, "\\nleft_child: NO_LEFT_CHILD");

    if (n->right != NULL)
        fprintf(out, "\\nright_child: %d", n->right->key);
    else
        fprintf(out, "\\nright_child: NO_RIGHT_CHILD");
    fprintf(out, "\"];\n");

    (*qur_num)++;
    node_dump_graph(n->left, qur_num, out);
    for (int i = 0; i < 3; i++) {
        fprintf(out, "  cell%d [style=\"invis\", width=1, fixedsize=true];\n", *qur_num);  // средний невидимка
        (*qur_num)++;
    }
    node_dump_graph(n->right, qur_num, out);
    return TREE_OK;
}

// ======================================= построение одной стрелки ===============================================
static void gv_edge(FILE* o, const char* mode, int from, const char* fport, int to, const char* tport,
                    const char* color, const char* attrs) {
    if (attrs == NULL) {
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"];\n", from, to, color);
        return;
    }
    const char* sep = attrs[0] ? ", " : "";
    if (strcmp(mode, "ARCS") == 0)
        fprintf(o, "  cell%d:%s -> cell%d:%s [color=\"%s\"%s%s];\n",
                from, fport, to, tport, color, sep, attrs);
    else {
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"%s%s];\n", from, to, color, sep, attrs);
    }
}

// ======================================= построение всех стрелок для текущей ячейки ====================================
tree_err_t arrow_dump_graph(const Node* n, int* qur_num, FILE* out) {
    if (n == NULL) return TREE_OK;
    if (*qur_num > 4 * MAX_OF_NODES) return TREE_CORRYPTED;
    int parent = (*qur_num)++;

    if (n->left != NULL) {
        int left_child = *qur_num;
        arrow_dump_graph(n->left, qur_num, out);

        if (n->left->type == NON_TYPE && n->left->head == n)  // не уверен есть ли тут какие то проблемные случаи
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_FREE_CHAIN, "style=dashed, headclip=true");
        else if (n->left->head == n)  // left стрелка обычная или красная при несоглассованности
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_LEFT, "headclip=true");
        else
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_BAD, "penwidth=4, headclip=true");
    }

    for (int i = 0; i < 3; i++) {
        int invis_child = (*qur_num)++;
        gv_edge(out, TREE_GV_MODE, parent, "s", invis_child, "n", GV_FREE_CHAIN, "style=invis, weigh=10000, headclip=true");  // невидимый ребенок - его рисуем в любом случае
    }

    if (n->right != NULL) {
        int right_child = *qur_num;
        arrow_dump_graph(n->right, qur_num, out);

        if (n->right->type == NON_TYPE && n->right->head == n)  // не уверен есть ли тут какие то проблемные случаи
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_FREE_CHAIN, "style=dashed, headclip=true");
        else if (n->right->head == n)  // right стрелка обычная или красная при несогласованности
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_RIGHT ":" GV_RIGHT, "headclip=true");
        else
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_BAD, "penwidth=4, headclip=true");
    }
    return TREE_OK;
}

static void html_escape(FILE* o, const char* s) {
    for (; *s; s++) {
        switch (*s) {
            case '&':
                fputs("&amp;", o);
                break;
            case '<':
                fputs("&lt;", o);
                break;
            case '>':
                fputs("&gt;", o);
                break;
            default:
                fputc(*s, o);
        }
    }
}

tree_err_t tree_dump_graph_at(const char* base_name, const Tree* tree,
                              const char* call_file, int call_line, const char* call_func) {
    if (tree == NULL || base_name == NULL) return TREE_NULL_PTR;

    char dot[256], png[256];
    snprintf(dot, sizeof(dot), "./dot_of_dumps/%s_ortho.dot", base_name);
    snprintf(png, sizeof(png), "./png_dumps/%s_ortho.png", base_name);
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

    const char* mode_str = TREE_GV_MODE;
    fprintf(log_file, "<div class=\"dump-card\">\n");
    fprintf(log_file,
            "  <div class=\"meta\">Dump called from <span class=\"meta-func\">%s()</span> "
            "at <span class=\"meta-file\">%s:%d</span></div>\n",
            call_func, call_file, call_line);
#ifdef TREE_DEBUG
    fprintf(log_file,
            "  <div class=\"meta\">Tree <span class=\"meta-func\">%s</span> created in "
            "<span class=\"meta-func\">%s()</span> at <span class=\"meta-file\">%s:%d</span></div>\n",
            tree->name ? tree->name : "?",
            tree->function ? tree->function : "?",
            tree->file ? tree->file : "?",
            tree->line);
#endif  // печать места создания дерева

    const char* fail_file;
    const char* fail_func;
    const char* fail_expr;
    int fail_line;
    if (assert_get_place(&fail_file, &fail_line, &fail_func, &fail_expr)) {
        fprintf(log_file, "  <div class=\"error\"><b>ASSERT FAILED</b> at %s:%d in %s()<br>condition: <code>",
                fail_file, fail_line, fail_func);
        html_escape(log_file, fail_expr);
        fprintf(log_file, "</code></div>\n");
    }  // печать места падения

    fprintf(log_file, "  <div class=\"meta\"><strong>DUMP #%d</strong> | Mode: <span class=\"meta-func\">%s</span> | File: <span class=\"meta-file\">%s</span></div>\n",
            dump_id, mode_str, png);
    fprintf(log_file, "  <table class=\"stats-table\">\n");
    if (tree->root != NULL) {
        fprintf(log_file,
                "    <tr>"
                "<td class=\"var-name\">Size:</td><td class=\"var-val\">%zu</td>"
                "<td class=\"var-name\">Key of root:</td><td class=\"var-val\">%d</td>"
                "<td class=\"var-name\">Data of root:</td><td class=\"var-val\">",
                tree->size, tree->root->key);
        print_data(log_file, tree->root);
        fprintf(log_file, "</td>");
        if (tree->root->left != NULL) {
            fprintf(log_file,
                    "<td class=\"var-name\">Key of left child:</td><td class=\"var-val\">%d</td>", tree->root->left->key);
        }
        if (tree->root->right != NULL) {
            fprintf(log_file,
                    "<td class=\"var-name\">Key of right child:</td><td class=\"var-val\">%d</td>", tree->root->right->key);
        }
    } else {
        fprintf(log_file, "    <tr><td class=\"var-name\">Size:</td><td class=\"var-val\">0 (Empty tree)</td>\n");
    }
    fprintf(log_file, "</tr>\n");
    fprintf(log_file, "  </table>\n");

    // ============================ заголовок =========================
    fprintf(out, "digraph tree {\n");
    const char* splines = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? "ortho" : "true";
    double nodesep      = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? 2.0 : 1.6;
    double ranksep      = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? 2.5 : 1.5;
    fprintf(out, "  graph [dpi=110, bgcolor=\"" GV_BG
                 "\", splines=%s, pad=0.5,"
                 " nodesep=%.1f, ranksep=%.1f];\n",
            splines, nodesep, ranksep);
    fprintf(out, "  node  [shape=rectangle, style=\"rounded,filled\", fontcolor=\"" GV_TEXT
                 "\","
                 " fontsize=22, width=6.2, height=2.5, fixedsize=true, penwidth=3];\n");
    fprintf(out, "  edge  [penwidth=2, arrowsize=1.0];\n");

    //==================================================================================

    // ============================== ячейки ==========================
    int qur_num = 0;
    err         = node_dump_graph(tree->root, &qur_num, out);
    ASSERT(err == TREE_OK);

    // =========================== стрелки ============================
    qur_num = 0;
    err     = arrow_dump_graph(tree->root, &qur_num, out);
    ASSERT(err == TREE_OK);

    // =========== все ячейки строятся сверху вниз ================
    fprintf(out, "  { rank=TB;");
    for (size_t i = 0; i < tree->size; i++) fprintf(out, " cell%zu;", i);
    fprintf(out, " }\n");

    fprintf(out, "}\n");
    fclose(out);

    // ================================ запуск dot ===================================
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd), "dot -Tpng \"%s\" -o \"%s\"", dot, png);
    if (system(cmd) != 0) {
        fprintf(log_file, "  <div style=\"color: #e06c75; font-weight: bold;\">[ERROR] Graphviz generation failed!</div>\n");
        fprintf(log_file, "</div>\n");  // Закрываем dump-card
        fclose(log_file);
        return TREE_DOT_CANT_OPEN;
    }

    fprintf(log_file, "  <div class=\"graph-container\">\n");
    fprintf(log_file, "    <img src=\"%s\" alt=\"List State\">\n", png);
    fprintf(log_file, "  </div>\n");
    fprintf(log_file, "</div>\n");  // Закрываем dump-card

    fclose(log_file);

    return TREE_OK;
}

void tree_dump_hook(const char* name, void* ctx) {
    tree_dump_graph(name, (const Tree*)ctx);
}

tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit) {
    if (!n) return TREE_OK;
    if (++*count > limit) return TREE_WRONG_SIZE;
    if (n->type < 0 || n->type >= NUMBER_OF_TYPES) return TREE_WRONG_NODE_TYPE;
    if (n->type == STRING_TYPE && n->data.string_data == NULL) return TREE_WRONG_NODE_TYPE;
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
    if (t->root && t->root->head != NULL) return TREE_WRONG_HEAD;
    tree_err_t tree_err = verify_node(t->root, NULL, NULL, &count, t->size);
    if (tree_err != TREE_OK) return tree_err;
    if (count != t->size) return TREE_WRONG_SIZE;
    return TREE_OK;
}

tree_err_t node_inorder(FILE* o, const Node* n) {
    ASSERT(o != NULL);
    ASSERT(n != NULL);

    if (n->left != NULL) node_inorder(o, n->left);
    fprintf(o, "key: %d\ndata: ", n->key);
    print_data(o, n);
    fprintf(o, "\n\n");
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

int bfs(const Tree* t) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));
    if (t->root == NULL) return 0;

    int width = 1;
    list_t queue;
    list_err_t err = list_ctor(&queue, t->size);
    ASSERT(err == LIST_OK);

    err = list_elem_push(&queue, t->root, queue.prev[0], NULL);
    ASSERT(err != LIST_OK);
    int prev_width = 1;
    int next_width = 0;

    while (prev_width != 0) {
        while (prev_width != 0) {
            size_t front      = queue.next[0];
            Node* p           = queue.data[front];
            Node* left_child  = p->left;
            Node* right_child = p->right;
            if (left_child != NULL) {
                list_elem_push(&queue, left_child, queue.prev[0], NULL);
                next_width++;
            }
            if (right_child != NULL) {
                list_elem_push(&queue, right_child, queue.prev[0], NULL);
                next_width++;
            }
            list_elem_pop(&queue, front);
            prev_width--;
        }
        if (next_width > width) width = next_width;
        prev_width = next_width;
        next_width = 0;
    }

    list_err_t list_err = list_dtor(&queue);
    ASSERT(list_err == LIST_OK);

    return width;
}

/* был root он один мы его записали в список и обновили width
    далее достаем root из очереди и пихаем туда ео детей и обновили width
    далее достаем правого ребенка и записываем туда его детей, достаем 

    то есть есть queue мы запоминаем число детей root - 2 и достаем ровно столько и кладем детей и считаем сколько кладем
    далее снова снимаем ровно столько сколлько запомнили и кладем новые и это значение снова запоминаем
*/
void tree_log_init() {
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
            "        max-height: 1000px;\n"
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

static void renumber_node(Node* n, int* counter) {
    if (!n) return;
    renumber_node(n->left, counter);
    n->key = (*counter)++;
    renumber_node(n->right, counter);
}

void tree_renumber(Tree* t) {
    ASSERT(t != NULL);
    int counter = 0;
    renumber_node(t->root, &counter);
}