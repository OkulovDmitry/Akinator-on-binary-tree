#ifndef MY_ASSERT_H
#define MY_ASSERT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

// _Noreturn - функция не возврщает управление никогда, программа гарантированно заврешается внутри нее

// fmt               - доп сообщение от пользователя
// va_list ap        - создает спец указатель для обхода скрытых аргументов
// va_start(ap, fmt) - инициализирует указатель ap для обхода скрытых аргументов многоточия, аргументы ищет в памяти после последнего не скрытого - fmt
// vfprintf(...)     - принимает список аргументов из ap, читает строку формата fmt и печатает все в stderr
// fflush()          - принудительно выталкивает все, что мы только что написали в файл логов или на экран - чтобы при падении проги текст ошибки точно не потерялся

typedef void (*AssertDumpFn)(FILE* out, void* ctx);

/* Что вызвать перед смертью, NULL чтобы снять хук */
void assert_set_hook(AssertDumpFn fn, void* ctx);

_Noreturn void assert_fail(const char* file, int line,
                           const char* func, const char* expr,
                           const char* fmt, ...);

/* ---- VERIFY работает и в debug и в release ---- */
#define VERIFY(cond)                                                \
    do {                                                            \
        if (!(cond))                                                \
            assert_fail(__FILE__, __LINE__, __func__, #cond, NULL); \
    } while (0)

#define VERIFY_MSG(cond, ...)                                              \
    do {                                                                   \
        if (!(cond))                                                       \
            assert_fail(__FILE__, __LINE__, __func__, #cond, __VA_ARGS__); \
    } while (0)

/* ---- ASSERT только в DEBUG ---- */
#ifndef NDEBUG
#    define ASSERT(cond) VERIFY(cond)
#    define ASSERT_MSG(cond, ...) VERIFY_MSG(cond, __VA_ARGS__)
#else
#    define ASSERT(cond) ((void)sizeof(cond))  // (void)sizeof(cond) - убирает предупреждение unused variable
#    define ASSERT_MSG(cond, ...) ((void)sizeof(cond))
#endif /* NDEBUG */

/* Ассерт с одноразовым дампом конкретного объекта */
#define ASSERT_DUMP(cond, fn, obj)                                  \
    do {                                                            \
        if (!(cond)) {                                              \
            assert_set_hook((fn), (obj));                           \
            assert_fail(__FILE__, __LINE__, __func__, #cond, NULL); \
        }                                                           \
    } while (0)

/* ---- "сюда попасть нельзя" ---- */
#define UNREACHABLE() \
    assert_fail(__FILE__, __LINE__, __func__, "ureachable code reached", NULL)

/* ---- проверка на этапе компиляции ---- */
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#endif /* MY_ASSERT_H */