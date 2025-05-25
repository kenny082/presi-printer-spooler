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

#define SUITE jobs_suite

/*---------------------------test cancel job cmd--------------------------------*/
#define TEST_NAME cancel_job_test
#define type_cmd "type aaa"
#define print_cmd "print test_scripts/testfile.aaa"
#define cancel_cmd "cancel 0"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,          timeout,    before,    after
    {  NULL,                INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,            TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  print_cmd,           JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  cancel_cmd,          JOB_ABORTED_EVENT,          EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  "quit",              FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd
#undef print_cmd
#undef cancel_cmd
#undef TEST_NAME

/*---------------------------test deleting a job --------------------------------*/
#define TEST_NAME delete_job
#define type_a  "type aaa"
#define printer "printer Alice aaa"
#define enable  "enable Alice"
#define print   "print test_scripts/testfile.aaa"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,        expect,                               modifiers,                         timeout,    before,    after
    {  NULL,        INIT_EVENT,               0,                              HND_MSEC,   NULL,      NULL },
    {  type_a,      TYPE_DEFINED_EVENT,       EXPECT_SKIP_OTHER,              HND_MSEC,   NULL,      NULL },
    {  printer,     PRINTER_DEFINED_EVENT,    EXPECT_SKIP_OTHER,              HND_MSEC,   NULL,      NULL },
    {  enable,      CMD_OK_EVENT,             EXPECT_SKIP_OTHER,              HND_MSEC,   NULL,      NULL },
    {  print,       JOB_FINISHED_EVENT,       EXPECT_SKIP_OTHER,              TWO_SEC,    NULL,      NULL },
    {  "jobs",      JOB_DELETED_EVENT,        EXPECT_SKIP_OTHER|DELAY_15SEC,  ZERO_SEC,   NULL,      NULL },
    {  "quit",      FINI_EVENT,               EXPECT_SKIP_OTHER,              HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,                0,                              TEN_MSEC,   NULL,      NULL }
};


Test(SUITE, TEST_NAME, .init = test_setup, .fini = test_teardown, .timeout = 20)
{
        int err, status;
        char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}

#undef TEST_NAME 
#undef type_a
#undef type_b  
#undef type_c  
#undef conv1
#undef conv2
#undef printer
#undef print
#undef enable  

/****************************************pause_resume_job*********************************/

#define TEST_NAME pause_resume_job
#define cmd1 "type aaa"
#define cmd3 "printer delay_printer aaa"
#define cmd4 "print test_scripts/testfileBIG.aaa"
#define cmd5 "jobs"
#define cmd6 "enable delay_printer"
#define cmd7 "pause 0"
#define cmd8 "printers" 
#define cmd9 "resume 0"
#define cmd10 "quit"

static COMMAND SCRIPT(TEST_NAME)[] = 
{

    //send,             expected,               modifiers,            timeout,   before,   after
    { NULL,             INIT_EVENT,             EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,    NULL},
    { cmd1,             TYPE_DEFINED_EVENT,     EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,    NULL},
    { cmd3,             PRINTER_DEFINED_EVENT,  EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,    NULL},
    { cmd4,             JOB_CREATED_EVENT,      EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd6,             JOB_STARTED_EVENT,      EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd7,             JOB_STATUS_EVENT,       EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd5,             CMD_OK_EVENT,           EXPECT_SKIP_OTHER,    ONE_MSEC,   NULL,    NULL},
    { cmd8,             CMD_OK_EVENT,           EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd9,             JOB_STATUS_EVENT,       EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd5,             JOB_FINISHED_EVENT,     EXPECT_SKIP_OTHER,    ZERO_SEC,   NULL,    NULL},
    { cmd10,            FINI_EVENT,             EXPECT_SKIP_OTHER,    HND_MSEC,   NULL,    NULL},
    { NULL,             EOF_EVENT,              0,                    HND_MSEC,   NULL,    NULL}
};

Test(SUITE, TEST_NAME, .init=test_delay_setup, .fini=test_teardown, .timeout=10){
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}


#undef TEST_NAME
#undef cmd1
#undef cmd2
#undef cmd3
#undef cmd4
#undef cmd5
#undef cmd6
#undef cmd7
#undef cmd8
#undef cmd9
#undef cmd10

#undef type1
#undef type2
#undef type3
#undef type4
#undef type5
#undef conv1
#undef conv2
#undef conv3
#undef conv4
#undef printer
#undef enable1
#undef job1
#undef quit
#undef TEST_NAME


/**************************** Pause Multiple jobs, resume, and use large files *****************/

#define TEST_NAME multi_pause_multi_resume
#define type1 "type aaa"
#define type2 "type bbb"
#define printer1 "printer pA aaa"
#define printer2 "printer pB bbb"
#define printer3 "printer pB2 bbb"
#define enable1 "enable pA"
#define enable2 "enable pB"
#define enable3 "enable pB2"
#define job0 "print test_scripts/testfileBIG.aaa"
#define job1 "print test_scripts/testfileBIG.bbb pB"
#define pause0 "pause 0"
#define pause1 "pause 1"
#define job2 "print test_scripts/testfileBIG.bbb pB2"
#define pause2 "pause 2"
#define resume0 "resume 0"
#define resume1 "resume 1"
#define jobs "jobs"
#define resume2 "resume 2"
#define printers "printers"
#define quit "quit"

static COMMAND SCRIPT(TEST_NAME)[] =
{
    //send,       expected,                 modifiers,                     timeout, before, after
    { NULL,       INIT_EVENT,               EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { type1,      TYPE_DEFINED_EVENT,       EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { type2,      TYPE_DEFINED_EVENT,       EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { printer1,   PRINTER_DEFINED_EVENT,    EXPECT_SKIP_OTHER,             TEN_MSEC,  NULL, NULL},
    { printer2,   PRINTER_DEFINED_EVENT,    EXPECT_SKIP_OTHER,             TEN_MSEC,  NULL, NULL},
    { printer3,   PRINTER_DEFINED_EVENT,    EXPECT_SKIP_OTHER,             TEN_MSEC,  NULL, NULL},
    { enable1,    PRINTER_STATUS_EVENT,     EXPECT_SKIP_OTHER,             TEN_MSEC,  NULL, NULL},
    { enable2,    PRINTER_STATUS_EVENT,     EXPECT_SKIP_OTHER,             TEN_MSEC,  NULL, NULL},
    { enable3,    PRINTER_STATUS_EVENT,     EXPECT_SKIP_OTHER,             ONE_MSEC,  NULL, NULL},
    { job0,       JOB_CREATED_EVENT,        EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { pause0,     JOB_STATUS_EVENT,         EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { job1,       JOB_CREATED_EVENT,        EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { pause1,     JOB_STATUS_EVENT,         EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { jobs,       CMD_OK_EVENT,             EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { job2,       JOB_CREATED_EVENT,        EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { pause2,     JOB_STATUS_EVENT,         EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { jobs,       CMD_OK_EVENT,             EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { resume0,    JOB_FINISHED_EVENT,       EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { resume1,    JOB_FINISHED_EVENT,       EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { resume2,    JOB_FINISHED_EVENT,       EXPECT_SKIP_OTHER,             ZERO_SEC,  NULL, NULL},
    { quit,       FINI_EVENT,               EXPECT_SKIP_OTHER,             HND_MSEC,  NULL, NULL},
    { NULL,       EOF_EVENT,                0,                             TEN_MSEC,  NULL, NULL}
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini=test_teardown, .timeout=20){
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}

#undef TEST_NAME
#undef type1
#undef type2
#undef conversion1
#undef printer1
#undef printer2
#undef job0
#undef job1
#undef pause0
#undef pause1
#undef job2
#undef pause2
#undef resume0
#undef resume1
#undef jobs
#undef printers
#undef quit
