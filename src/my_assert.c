#include "my_assert.h"

static AssertDumpFn g_hook;
static void* g_ctx;

void assert_set_hook(AssertDumpFn fn, void* ctx) {
    g_hook = fn;
    g_ctx  = ctx;
}

_Noreturn void assert_fail(const char* file, int line,
                           const char* func, const char* expr,
                           const char* fmt, ...) {
    static int in_fail; /* защита от рекурсии */
    if (in_fail) abort();
    in_fail = 1;

    FILE* outs[3] = {stderr, fopen("errors.log", "a"), fopen("log.html", "a")};

    for (int i = 0; i < 2; i++) {
        FILE* o = outs[i];
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
            g_hook(o, g_ctx);
            fputs(" ------------------------------- end dump -------------------------------\n", o);
        }
        fflush(o);
    }

    if (outs[2]) {
        fprintf(outs[2], "<div class=\"error\"><b>ASSERT FAILED: %s</b><br>%s:%d: <code>",
                expr, file, line);

        // html_escape
        for (; *expr; expr++) {
            switch (*expr) {
                case '&':
                    fputs("&amp;", outs[2]);
                    break;
                case '<':
                    fputs("&lt;", outs[2]);
                    break;
                case '>':
                    fputs("&gt;", outs[2]);
                    break;
                default:
                    fputc(*expr, outs[2]);
            }
        }

        fprintf(outs[2], "</code></div>\n");
    }

    if (outs[1]) fclose(outs[1]);
    if (outs[2]) fclose(outs[2]);

    system("start chrome \"%CD%\\log.html\"");

    abort();
}