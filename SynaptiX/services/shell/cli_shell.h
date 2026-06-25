/**
 * @file cli_shell.h
 * @brief Lightweight CLI shell — ported from RS485_v2, transport-agnostic.
 *
 * The shell is decoupled from the physical transport (USB CDC, UART, etc).
 * Callers provide send_char / send_str callbacks via sCliShellImpl_t.
 *
 * Commands are registered via the linker-section pattern:
 *   extern const Cli_Shell_Cmd *const g_shell_commands;
 *   extern const size_t               g_num_shell_commands;
 * Define these in shell_commands.c (app layer).
 */

#ifndef CLI_SHELL_H
#define CLI_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define SHELL_RX_BUFFER_SIZE    256
#define SHELL_MAX_ARGS          16
#define SHELL_PROMPT            "synaptix> "

typedef struct Cli_Shell_Cmd_t  Cli_Shell_Cmd;
typedef struct Cli_ShellImpl    sCliShellImpl_t;
typedef struct ShellContext     ShellContext_t;

struct Cli_Shell_Cmd_t {
    const char *command;
    int (*handler)(ShellContext_t *shell, int argc, char *argv[]);
    const char *help;
};

struct Cli_ShellImpl {
    void *arg;
    int (*send_char)(void *arg, char c);
    int (*send_str)(void *arg, char *s);
};

struct ShellContext {
    sCliShellImpl_t impl;
    size_t          rx_size;
    char            rx_buffer[SHELL_RX_BUFFER_SIZE];
};

/* ── API ──────────────────────────────────────────────────────────────────── */

/** Initialise shell and print prompt. Call once after impl is populated. */
void cli_shell_boot(ShellContext_t *shell);

/** Feed one received character into the shell. Call from RX task/ISR. */
void cli_shell_receive_char(ShellContext_t *shell, char c);

/** Print a string followed by newline. */
void cli_shell_put_line(ShellContext_t *shell, const char *str);

/** printf-style output to shell. */
void cli_shell_printf(ShellContext_t *shell, const char *fmt, ...);

/** Built-in 'help' command handler — register in g_shell_commands. */
int cli_shell_help_handler(ShellContext_t *shell, int argc, char *argv[]);

/* Defined by app layer (shell_commands.c) */
extern const Cli_Shell_Cmd  *const g_shell_commands;
extern const size_t                 g_num_shell_commands;

#ifdef __cplusplus
}
#endif

#endif /* CLI_SHELL_H */