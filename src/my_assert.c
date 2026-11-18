#include "my_assert.h"

static AssertDumpFn g_hook;
static void* g_ctx;

static int in_fail; /* защита от рекурсии */
static const char* g_file;
static int g_line;
static const char* g_func;
static const char* g_expr;

int assert_get_place(const char** file, int* line, const char** func, const char** expr) {
    if (!in_fail) return 0;
    *file = g_file;
    *line = g_line;
    *func = g_func;
    *expr = g_expr;
    return 1;
}

void assert_set_hook(AssertDumpFn fn, void* ctx) {
    g_hook = fn;
    g_ctx  = ctx;
}

_Noreturn void assert_fail(const char* file, int line,
                           const char* func, const char* expr,
                           const char* fmt, ...) {
    if (in_fail) abort();
    in_fail = 1;
    g_file  = file;
    g_line  = line;
    g_func  = func;
    g_expr  = expr;

    char* outs[3] = {"stderr", "errors.log", "log.html"};

    for (int i = 0; i < 2; i++) {
        FILE* o;
        if (strcmp(outs[i], "stderr") == 0)
            o = stderr;
        else
            o = fopen(outs[i], "a");
        if (!o) continue;

        fprintf(o, "\nASSERT FAILED: %s\n  at %s:%d in %s()\n",
                expr, file, line, func);

        if (fmt) {
            va_list ap;
            va_start(ap, fmt);
            fputs(" msg: ", o);
            vfprintf(o, fmt, ap);
            fputc('\n', o);
            va_end(ap);
        }
        if (g_hook) {
            fputs(" --------------------------------- dump ---------------------------------\n", o);
            if (strcmp(outs[i], "stderr") != 0) fclose(o);
            g_hook(outs[i], g_ctx);
            if (strcmp(outs[i], "stderr") == 0)
                o = stderr;
            else
                o = fopen(outs[i], "a");
            fprintf(stderr, "I WAS HERE\n");
            if (!o) continue;
            fputs(" ------------------------------- end dump -------------------------------\n", o);
            fflush(o);
        }
        fflush(o);
        if (strcmp(outs[i], "stderr") != 0) fclose(o);
    }

    FILE* o;
    if (strcmp(outs[2], "stderr") == 0)
        o = stderr;
    else
        o = fopen(outs[2], "a");

    if (outs[2] != NULL) {
        fprintf(o, "<div class=\"error\"><b>ASSERT FAILED: %s</b><br>%s:%d: <code>",
                expr, file, line);

        // html_escape
        for (; *expr; expr++) {
            switch (*expr) {
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
                    fputc(*expr, o);
            }
        }

        fprintf(o, "</code></div>\n");
        fclose(o);
    }

    system("start chrome \"%CD%\\log.html\"");

    abort();
}