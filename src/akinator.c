#include <stdbool.h>
#include <stdio.h>

#include "tree.h"

int check_bufer(void);
void clear_bufer(void);
tree_err_t database_choose(Tree* t, char* name);
int turn_on(void);
char node_answer(void);
void node_printf(Node* n);
bool akinator_new_question(Node** p);
void akinator_win();
void akinator_failed(Tree* t, Node** problem_list, const char* name);
void skip_comments(FILE* akinator_characteristic);
void skip_end_of_line_comment(FILE* akinator_characteristic);

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    tree_log_init();

    printf("Select a database for Akinator.\n");
    Tree t = {0};
    assert_set_hook(tree_dump_hook, &t);  // любой упавший ассерт сбросит дерево в лог

    char database_name[256] = "POISON";
    tree_err_t err          = database_choose(&t, database_name);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

    err = tree_dump_graph("NOW_Akinator_Database", &t);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

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

    tree_destroy(&t);

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
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));
    ASSERT_MSG(name != NULL, tree_strerror(NULL_PTR));

    scanf("%255s", name);
    FILE* database = fopen(name, "r");

    if (!database) {
        printf("This database does not exist.\n");
        printf("Try again: \n");
        clear_bufer();
        return database_choose(t, name);
    } else {
        fclose(database);
        TREE_INIT(t);
        printf("Selection in progress...\n");
        tree_err_t read_err = read_rpn_tree(t, name);
        printf("read_err = %d, root = %p\n", read_err, (void*)t->root);
        fflush(stdout);
        if (read_err != TREE_OK || t->root == NULL) {
            return read_err;
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
    ASSERT_MSG(n != NULL, tree_strerror(NODE_NULL_PTR));

    printf("%s", n->data.string_data);
    printf("?\n");
}

bool akinator_new_question(Node** p) {
    ASSERT_MSG(p != NULL, tree_strerror(NODE_NULL_PTR));

    node_printf(*p);
    char answer = node_answer();
    ASSERT_MSG(answer == 'y' || answer == 'n', "error: answer in wrong format, please use y or n");

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

void skip_end_of_line_comment(FILE* akinator_characteristic) {
    int symbol = fgetc(akinator_characteristic);
    if (symbol == ' ') {
        symbol = fgetc(akinator_characteristic);
        if (symbol == '#') {
            while (symbol != '\n') {
                symbol = fgetc(akinator_characteristic);
            }
        }
    }

    ungetc(symbol, akinator_characteristic);
}

void skip_comments(FILE* akinator_characteristic) {
    int symbol = fgetc(akinator_characteristic);
    while (symbol == '#') {
        while (symbol != '\n') {
            symbol = fgetc(akinator_characteristic);
        }
        symbol = fgetc(akinator_characteristic);
    }

    ungetc(symbol, akinator_characteristic);
}

void akinator_win() {
    printf("Great! Guessed right one more time!\n");
    FILE* akinator_characteristic = fopen("Akinator_database/akinator_characteristic.txt", "r");
    int number_of_sucess = 0, number_of_bronze = 0, number_of_silver = 0, number_of_gold = 0;
    skip_comments(akinator_characteristic);
    fscanf(akinator_characteristic, "%i", &number_of_sucess);
    skip_end_of_line_comment(akinator_characteristic);
    skip_comments(akinator_characteristic);
    fscanf(akinator_characteristic, "%i", &number_of_bronze);
    skip_end_of_line_comment(akinator_characteristic);
    skip_comments(akinator_characteristic);
    fscanf(akinator_characteristic, "%i", &number_of_silver);
    skip_end_of_line_comment(akinator_characteristic);
    skip_comments(akinator_characteristic);
    fscanf(akinator_characteristic, "%i", &number_of_gold);

    number_of_sucess++;
    printf("I won %i times\n", number_of_sucess);
    printf("You have %i bronze medals\n", number_of_bronze);
    printf("You have %i silver medals\n", number_of_silver);
    printf("You have %i gold medals\n", number_of_gold);

    fclose(akinator_characteristic);
    akinator_characteristic = fopen("Akinator_database/akinator_characteristic.txt", "w");
    fprintf(akinator_characteristic, "%i # number of sucess\n", number_of_sucess);
    fprintf(akinator_characteristic, "%i # number of bronze\n", number_of_bronze);
    fprintf(akinator_characteristic, "%i # number of silver\n", number_of_silver);
    fprintf(akinator_characteristic, "%i # number of gold\n", number_of_gold);
    fclose(akinator_characteristic);
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
    character_node->type             = STRING_TYPE;
    character_node->left             = NULL;
    character_node->right            = NULL;

    Node* new_node             = malloc(sizeof(Node));
    new_node->data.string_data = strdup(trait);
    new_node->head             = (*problem_list)->head;
    new_node->key              = 0;
    new_node->type             = STRING_TYPE;
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
    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    tree_err_t graph_err = tree_dump_graph("Akinator_after_add", t);
    ASSERT_MSG(graph_err == TREE_OK, tree_strerror(graph_err));
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