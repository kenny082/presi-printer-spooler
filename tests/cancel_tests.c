#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <sys/time.h>

#include "driver.h"
#include "__helper.h"

#define QUOTE1(x) #x
#define QUOTE(x) QUOTE1(x)
#define SCRIPT1(x) x##_script
#define SCRIPT(x) SCRIPT1(x)

#define SUITE cancel_suite 

#define TEST_NAME cancel_running_job
#define type1_cmd   "type aaa"
#define type2_cmd   "type bbb"
#define conv_cmd    "conversion aaa bbb util/convert_slow aaa bbb"
#define print_cmd   "print test_scripts/testfile.aaa"
#define printer     "printer Alice1 bbb"
#define enable_cmd  "enable Alice1"
#define cancel_cmd  "cancel 0"
#define quit_cmd    "quit"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,            timeout,  before,    after
    {  NULL,                INIT_EVENT,                 0,                    HND_MSEC,   NULL,      NULL },
    {  type1_cmd,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  type2_cmd,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  conv_cmd,            CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  print_cmd,           JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  printer,             PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  enable_cmd,          NO_EVENT,                   DELAY_1SEC,           HND_MSEC,   NULL,      NULL },
    {  cancel_cmd,          JOB_ABORTED_EVENT,          EXPECT_SKIP_OTHER,    TEN_SEC,    NULL,      NULL },
    {  quit_cmd,            FINI_EVENT,                 EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                    TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init = test_delay_setup, .fini = test_teardown, .timeout = 10)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type1_cmd   
#undef type2_cmd
#undef conv_cmd
#undef print_cmd
#undef printer
#undef enable_cmd 
#undef cancel_cmd
#undef quit_cmd
#undef TEST_NAME

#define TEST_NAME cancel_paused_job
#define type1_cmd   "type aaa"
#define type2_cmd   "type bbb"
#define conv_cmd    "conversion aaa bbb util/convert_slow aaa bbb"
#define print_cmd   "print test_scripts/testfile.aaa"
#define printer     "printer Alice1 bbb"
#define enable_cmd  "enable Alice1"
#define pause_cmd   "pause 0"
#define cancel_cmd  "cancel 0"
#define quit_cmd    "quit"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,            timeout,  before,    after
    {  NULL,                INIT_EVENT,                 0,                    HND_MSEC,   NULL,      NULL },
    {  type1_cmd,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  type2_cmd,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  conv_cmd,            CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  print_cmd,           JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  printer,             PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  enable_cmd,          NO_EVENT,                   DELAY_1SEC,           HND_MSEC,   NULL,      NULL },
    {  pause_cmd,           JOB_STATUS_EVENT,           EXPECT_SKIP_OTHER,    TEN_SEC,    NULL,      NULL },
    {  cancel_cmd,          JOB_ABORTED_EVENT,          EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  quit_cmd,            FINI_EVENT,                 EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                    TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init = test_delay_setup, .fini = test_teardown, .timeout = 10)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type1_cmd   
#undef type2_cmd
#undef conv_cmd
#undef print_cmd
#undef printer
#undef enable_cmd 
#undef pause_cmd 
#undef cancel_cmd
#undef quit_cmd
#undef TEST_NAME

#define TEST_NAME cancel_finished_job
#define type_cmd    "type aaa"
#define print_cmd   "print test_scripts/testfile.aaa"
#define printer     "printer Alice1 aaa"
#define enable_cmd  "enable Alice1"
#define cancel_cmd  "cancel 0"
#define quit_cmd    "quit"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,            timeout,  before,    after
    {  NULL,                INIT_EVENT,                 0,                    HND_MSEC,   NULL,      NULL },
    {  type_cmd,            TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  print_cmd,           JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  printer,             PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  enable_cmd,          JOB_FINISHED_EVENT,         EXPECT_SKIP_OTHER,     TEN_SEC,   NULL,      NULL },
    {  cancel_cmd,          CMD_ERROR_EVENT,            EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  quit_cmd,            FINI_EVENT,                 EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                    TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init = test_delay_setup, .fini = test_teardown, .timeout = 10)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}

#undef TEST_NAME
#undef type_cmd
#undef print_cmd
#undef printer
#undef enable_cmd
#undef cancel_cmd
#undef quit_cmd

#define TEST_NAME resume_non_running_job
#define type_cmd     "type aaa"
#define printer_cmd  "printer Alice aaa"
#define print_cmd    "print test_scripts/testfile.aaa"
#define resume_cmd   "resume 0"
#define quit_cmd     "quit"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,          timeout,    before,    after
    {  NULL,                INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,            TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  printer_cmd,         PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  print_cmd,           JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  resume_cmd,          CMD_ERROR_EVENT,            EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,            FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init = test_delay_setup, .fini = test_teardown, .timeout = 10)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}

#undef TEST_NAME
#undef type_cmd
#undef printer_cmd
#undef print_cmd
#undef resume_cmd
#undef quit_cmd
