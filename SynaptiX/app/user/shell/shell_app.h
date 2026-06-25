/**
 * @file shell_app.h
 * @brief Shell application — wires cli_shell service to USB CDC interface 1.
 */

#ifndef SHELL_APP_H
#define SHELL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise shell: populate impl callbacks, boot cli_shell,
 *        spawn shell_receive_task FreeRTOS task.
 *        Call once from app_init() or defaultTask.
 */
void shell_app_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_APP_H */