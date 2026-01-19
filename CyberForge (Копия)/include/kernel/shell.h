#ifndef SHELL_H
#define SHELL_H

void shell_init(void);
void shell_run(void);
void history_command(void);
void add_to_history(const char* command);

#endif
