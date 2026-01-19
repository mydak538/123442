#ifndef COMMANDS_H
#define COMMANDS_H

void run_command(char* line);
void show_help(void);
void reboot_system(void);
void shutdown_system(void);
void cpu_command(void);
void coreview_command(void);
void date_command(void);
void time_command(void);
void biosver_command(void);
void osver_command(void);
void osinfo_command(void);
void memory_command(void);
void fsck_command(void);
void matrix_game(void);
void sphere_rand(void);
void math_game(void);
void calendar_command(void);
void writer_command(const char* filename);
void wexplorer_command(void);
void install_disk(void);
void config_command(void);
void watch_command(void);
void calc_command(const char* expression);
void autorun_command(const char* arg);
void autorun_execute(void);
void exit_command(void);
void find_command(const char* pattern);
void pwd_command(void);
void ps_command(void);
void kill_command(const char* arg);
void history_command(void);

#endif
