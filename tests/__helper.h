#ifndef __HELPER_H
#define __HELPER_H

#include <unistd.h>

#define TEST_EXECUTABLE "bin/presi","-t","-q"

void test_suite_setup(void);
void test_suite_teardown(void);
void test_setup(void);
void test_delay_setup(void);
void test_flaky_setup(void);
void test_teardown(void);

void get_formatted_cmd_input(char **args, int len, char *final_cmd);
char* get_printer_output_name(char *printer_name, char *printer_type);
void env_error_abort_test(char* msg);

void assert_proper_exit_status(int err, int status);
void assert_daemon_status_change(EVENT *ep, int *env, void *args);
void assert_num_daemons(EVENT *ep, int *env, void *args);
void assert_log_exists_check(char *logfile_name, int version_indicator);
void assert_diff_check(char *full_answer_logfile_path, char *logfile_name, int version_indicator);
void assert_valid_conversion(char *file_pattern, char *first_type, char *last_type);

#endif
