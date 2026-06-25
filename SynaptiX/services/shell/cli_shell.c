/**
 * @file cli_shell.c
 * @brief Lightweight CLI shell implementation — ported from RS485_v2.
 *
 * Changes from V2:
 *   - vsprintf -> vsnprintf (buffer overflow fix)
 *   - SHELL_PROMPT updated to "synaptix> "
 */

#include "cli_shell.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ── Internal helpers ─────────────────────────────────────────────────────── */

#define SHELL_FOR_EACH_COMMAND(cmd) \
    for (const Cli_Shell_Cmd *cmd = g_shell_commands; \
         cmd < &g_shell_commands[g_num_shell_commands]; \
         ++cmd)

static bool prv_booted(ShellContext_t *s)
{
    return s->impl.send_char != NULL;
}

static void prv_send_char(ShellContext_t *s, char c)
{
    if (!prv_booted(s)) return;
    s->impl.send_char(s->impl.arg, c);
}

static void prv_echo(ShellContext_t *s, char c)
{
    if (c == '\n') {
        prv_send_char(s, '\r');
        prv_send_char(s, '\n');
    } else if (c == '\b') {
        prv_send_char(s, '\b');
        prv_send_char(s, ' ');
        prv_send_char(s, '\b');
    } else {
        prv_send_char(s, c);
    }
}

static void prv_echo_str(ShellContext_t *s, const char *str)
{
    s->impl.send_str(s->impl.arg, (char *)str);
}

static void prv_send_prompt(ShellContext_t *s)
{
    prv_echo_str(s, SHELL_PROMPT);
}

static char prv_last_char(ShellContext_t *s)
{
    return s->rx_buffer[s->rx_size - 1];
}

static bool prv_is_rx_buffer_full(ShellContext_t *s)
{
    return s->rx_size >= SHELL_RX_BUFFER_SIZE;
}

static void prv_reset_rx_buffer(ShellContext_t *s)
{
    memset(s->rx_buffer, 0, sizeof(s->rx_buffer));
    s->rx_size = 0;
}

static const Cli_Shell_Cmd *prv_find_command(const char *name)
{
    SHELL_FOR_EACH_COMMAND(cmd) {
        if (strcmp(cmd->command, name) == 0) return cmd;
    }
    return NULL;
}

static void prv_process(ShellContext_t *s)
{
    if (prv_last_char(s) != '\n' && !prv_is_rx_buffer_full(s)) return;

    char *argv[SHELL_MAX_ARGS] = {0};
    int   argc = 0;
    char *next_arg = NULL;

    for (size_t i = 0; i < s->rx_size && argc < SHELL_MAX_ARGS; ++i) {
        char *const c = &s->rx_buffer[i];
        if (*c == ' ' || *c == '\n' || i == s->rx_size - 1) {
            *c = '\0';
            if (next_arg) {
                argv[argc++] = next_arg;
                next_arg = NULL;
            }
        } else if (!next_arg) {
            next_arg = c;
        }
    }

    if (s->rx_size == SHELL_RX_BUFFER_SIZE) {
        prv_echo(s, '\n');
    }

    if (argc >= 1) {
        const Cli_Shell_Cmd *cmd = prv_find_command(argv[0]);
        if (!cmd) {
            prv_echo_str(s, "Unknown command: ");
            prv_echo_str(s, argv[0]);
            prv_echo(s, '\n');
            prv_echo_str(s, "Type 'help' to list all commands\n");
        } else {
            cmd->handler(s, argc, argv);
        }
    }

    prv_reset_rx_buffer(s);
    prv_send_prompt(s);
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void cli_shell_boot(ShellContext_t *s)
{
    prv_reset_rx_buffer(s);
    prv_echo_str(s, "\n" SHELL_PROMPT);
}

void cli_shell_receive_char(ShellContext_t *s, char c)
{
    if (c == '\r' || prv_is_rx_buffer_full(s) || !prv_booted(s)) return;

    if (c == '\b') {
        if (s->rx_size > 0) {
            s->rx_buffer[--s->rx_size] = '\0';
        }
        return;
    }

    s->rx_buffer[s->rx_size++] = c;
    prv_process(s);
}

void cli_shell_put_line(ShellContext_t *s, const char *str)
{
    prv_echo_str(s, str);
    prv_echo(s, '\n');
}

static char s_printf_buf[256];

void cli_shell_printf(ShellContext_t *s, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_printf_buf, sizeof(s_printf_buf), fmt, args);  /* V2 bug fix: vsprintf → vsnprintf */
    va_end(args);
    cli_shell_put_line(s, s_printf_buf);
}

int cli_shell_help_handler(ShellContext_t *s, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    SHELL_FOR_EACH_COMMAND(cmd) {
        prv_echo_str(s, cmd->command);
        prv_echo_str(s, ": ");
        prv_echo_str(s, cmd->help);
        prv_echo(s, '\n');
    }
    return 0;
}