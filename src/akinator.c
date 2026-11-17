#include <stdbool.h>
#include <stdio.h>

#include "tree.h"

/* VERIFY работает и в release, в отличие от ASSERT */
/*static void insert_ok(Tree* t, int key) {
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
    }*/
// пока что лишнее от прошлой проги

int check_bufer(void);
void clear_bufer(void);
tree_err_t database_choose(Tree* t, char* name);
int turn_on(void);
char node_answer(void);
void node_printf(Node* n);
bool akinator_new_question(Node** p);
void akinator_win();
void akinator_failed(Tree* t, Node** problem_list, const char* name);

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    tree_log_init();

    printf("Select a database for Akinator.\n");
    Tree t;
    char database_name[256] = "POISON";
    tree_err_t err          = database_choose(&t, database_name);
    assert_set_hook(tree_dump_hook, &t);  // любой упавший ассерт сбросит дерево в лог
    if (err != TREE_OK) {
        printf("ERROR: Database selection failed.\n");
        return 1;
    }

    tree_dump_graph("NOW_Akinator_Database", &t);

    printf("======================================\nAKINATOR\n======================================\n");

    while (turn_on()) {
        Node* end_list = t.root;
        
        bool guess_result = akinator_new_question(&end_list);
        if (guess_result) {
            akinator_win();
        } else {
            akinator_failed(&t, &end_list, database_name);
        }
    }

    printf("Thank you for playing");

    system("start chrome \"%CD%\\log.html\"");

    return 0;
}

int check_buffer(void) {
    int buffer = getchar();
    if (buffer == '\n' || buffer == EOF) {
        return false;
    }

    return true;
}

void clear_bufer(void) {
    int buffer = 0;
    while ((buffer = getchar()) != '\n' && buffer != EOF);
}

tree_err_t database_choose(Tree* t, char* name) {
    //printf("I WAS HERE\n");
    scanf("%s", name);
    FILE* database = fopen(name, "r");
    //printf("I WAS HERE\n");
    if (!database) {
        printf("This database does not exist.\n");
        printf("Try again: \n");
        clear_bufer();
        fclose(database);
        return database_choose(t, name);
    } else {
        fclose(database);
        tree_init(t);
        printf("Selection in progress...\n");
        tree_err_t read_err = read_rpn_tree(t, name);
        printf("read_err = %d, root = %p\n", read_err, (void*)t->root);
        fflush(stdout);
        if (read_err != TREE_OK || t->root == NULL) {
            return TREE_IO_ERROR;
        }
        return TREE_OK;
    }
}

int turn_on(void) {
    int choice = 0;
    printf("Hi! I'm Akinator. Think of a real or fictional character, and I'll try to guess it.\n");
    printf("(0) Stop the program (1) Start\nEnter your choice: ");
    while (scanf("%i", &choice) != 1 || (choice != 0 && choice != 1) || check_buffer()) {
        printf("ERROR: Incorrect format\n");
        printf("Enter choice again: ");
        clear_bufer();
    }

    return choice;
}

char node_answer(void) {
    char choice = 0;
    while (scanf("%c", &choice) != 1 || (choice != 'y' && choice != 'n') || check_buffer()) {
        printf("ERROR: Incorrect format of answer\n");
        printf("Enter choice again: ");
        clear_bufer();
    }

    return choice;
}

void node_printf(Node* n) {
    ASSERT(n != NULL);

    printf("%s", n->data.string_data);
    printf("?\n");
}

bool akinator_new_question(Node** p) {
    ASSERT(p != NULL);

    node_printf(*p);
    char answer = node_answer();
    ASSERT(answer == 'y' || answer == 'n');

    if (answer == 'y') {
        if ((*p)->left == NULL) {
            return true;
        } else {
            *p = (*p)->left;
            return akinator_new_question(p);
        }
    } else {
        if ((*p)->right == NULL) {
            return false;
        } else {
            *p = (*p)->right;
            return akinator_new_question(p);
        }
    }
}

void akinator_win() {
    printf("Great! Guessed right one more time!\n");
}

void akinator_failed(Tree* t, Node** problem_list, const char* name) {
    printf("I give up!\n");
    printf("What characteristic distinguishes [New Character] from [Guessed Character]?\n");
    char trait[256] = "POISON";
    while (scanf("%s", trait) <= 0 || check_buffer()) {
        printf("ERROR: Incorrect format\n");
        printf("Enter choice again: ");
        clear_bufer();
    }
    printf("What name of [New Character]\n");
    char character[256] = "POISON";
    while (scanf("%s", character) <= 0) {
        printf("ERROR: Incorrect format\n");
        printf("Enter choice again: ");
        clear_bufer();
    }

    Node* character_node             = malloc(sizeof(Node));
    character_node->data.string_data = strdup(character);
    character_node->key              = 0;
    character_node->type = STRING_TYPE;
    character_node->left             = NULL;
    character_node->right            = NULL;

    Node* new_node             = malloc(sizeof(Node));
    new_node->data.string_data = strdup(trait);
    new_node->head             = (*problem_list)->head;
    new_node->key              = 0;
    new_node->type = STRING_TYPE;
    new_node->left             = character_node;
    new_node->right            = *problem_list;

    if ((*problem_list)->head == NULL) {
        t->root = new_node;
    } else if ((*problem_list)->head->left == *problem_list) {
        (*problem_list)->head->left = new_node;
    } else {
        (*problem_list)->head->right = new_node;
    }

    (*problem_list)->head = new_node;
    character_node->head  = new_node;

    (*problem_list)->left  = NULL;
    (*problem_list)->right = NULL;

    t->size += 2;

    //Akinator_database/first_database.txt

    tree_renumber(t);
    tree_dump_graph(name, t);
    tree_dump(name, t);

    printf("%s\n", name);

    printf("New character successfully saved.\n");
}

/* акинатор:
вывод приветствия: Привет! Я Акинатор. Загадай реального или вымышленного персонажа, а я попробую его отгадать
далее первый вопрос, если да - левый ребенок если нет - правый ребенок
далее если дошли до листа - листы - это предложения типа Это заяц? Это компьютер? и тд
Если же в листе да - то все угадали вывод следующих параметров:
• Статистика отгадываний: Игра показывает, сколько раз этого персонажа уже загадывали другие пользователи и когда его отгадали в последний раз.
• Награда (Аки-награда): Если персонаж редкий и его давно не загадывали, Акинатор присуждает специальную награду (Бронзовую, Серебряную, Золотую, Платиновую или Черную) и начисляет игровые баллы (Geniz).
• Кнопки дальнейших действий: Внизу появляются опции «Играть снова», «Поделиться результатом» или «Посмотреть фото» (в зависимости от версии приложения или сайта).

Ели же нет - акинатор говорит Вы меня победили и просит:
новую характеристику - и теперь наш лист сдвигается в левого ребенка на его место встает новая характеристика и справа будет новый ответ либо еще одна характеристика и тд но для начала там будет просто сразу лист*/