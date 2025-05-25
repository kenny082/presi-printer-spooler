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

#define SUITE conversion_suite
#define TEST_NAME one_conversion
#define type_a  "type aaa"
#define type_b  "type bbb"
#define type_c  "type ccc"
#define conv1   "conversion aaa bbb util/convert aaa bbb"
#define printer "printer Bob bbb"
#define enable  "enable Bob"
#define print   "print test_scripts/testfile.aaa"

static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,        expect,                   modifiers,          timeout,    before,    after
    {  NULL,        INIT_EVENT,               0,                  HND_MSEC,   NULL,      NULL },
    {  type_a,      TYPE_DEFINED_EVENT,       EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  type_b,      TYPE_DEFINED_EVENT,       EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  printer,     PRINTER_DEFINED_EVENT,    EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  conv1,       CONVERSION_DEFINED_EVENT, EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  enable,      CMD_OK_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  print,       JOB_FINISHED_EVENT,       EXPECT_SKIP_OTHER,  TWO_SEC,    NULL,      NULL },
    {  "quit",      FINI_EVENT,               EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,                0,                  TEN_MSEC,   NULL,      NULL }
};


Test(SUITE, TEST_NAME, .init = test_setup, .fini = test_teardown, .timeout = 15)
{
        int err, status;
        char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);

    assert_valid_conversion("spool/Bob_bbb_*", "aaa", "bbb");

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

/**************************** multiple_conversion ****************************/
#define TEST_NAME multiple_conversion
#define type1 "type aaa"
#define type2 "type bbb"
#define type3 "type ccc"
#define type4 "type pdf"
#define type5 "type txt"
#define conv1 "conversion aaa bbb util/convert aaa bbb"
#define conv2 "conversion bbb ccc util/convert bbb ccc"
#define conv3 "conversion ccc pdf util/convert ccc pdf"
#define conv4 "conversion pdf txt util/convert pdf txt"
#define printer "printer Check txt"
#define enable1 "enable Check"
#define job1 "print test_scripts/testfile1.aaa"
#define quit "quit"


static COMMAND SCRIPT(TEST_NAME)[] =
{
    //send,            expected,                   modifiers,           timeout,   before,  after
    { NULL,            INIT_EVENT,                 0,                   ONE_MSEC,   NULL,   NULL},
    { type1,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,   ONE_MSEC,   NULL,   NULL},
    { type2,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,   ONE_MSEC,   NULL,   NULL},
    { type3,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { type4,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,   ONE_MSEC,   NULL,   NULL},
    { type5,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { conv1,           CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { conv2,           CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { conv3,           CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { conv4,           CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { printer,         PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { enable1,         PRINTER_STATUS_EVENT,       EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { job1,            JOB_FINISHED_EVENT,         EXPECT_SKIP_OTHER,   ZERO_SEC,   NULL,   NULL},
    { quit,            FINI_EVENT,                 EXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { NULL,            EOF_EVENT,                  0,                   TEN_MSEC,   NULL,   NULL}
};


Test(SUITE, TEST_NAME, .init=test_setup, .fini=test_teardown, .timeout=20){
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    
    assert_valid_conversion("spool/Check_txt_*", "aaa", "txt");

    assert_proper_exit_status(err, status);
}

#undef TEST_NAME
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

#define TEST_NAME enable_off_conversion
#define type1    "type aaa"
#define type2    "type bbb"
#define printer  "printer Alice bbb"
#define job      "print test_scripts/testfile.aaa"
#define enable   "enable Alice"
#define conv     "conversion aaa bbb util/convert aaa bbb"
#define quit     "quit"

static COMMAND SCRIPT(TEST_NAME)[] =
{
    //send,            expected,                   modifiers,             timeout,   before,  after
    { NULL,            INIT_EVENT,                 0,                     ONE_MSEC,   NULL,   NULL},
    { type1,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,     TEN_MSEC,   NULL,   NULL},
    { type2,           TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,     TEN_MSEC,   NULL,   NULL},
    { printer,         PRINTER_DEFINED_EVENT,      EXPECT_SKIP_OTHER,     TEN_MSEC,   NULL,   NULL},
    { job,             JOB_CREATED_EVENT,          EXPECT_SKIP_OTHER,     HND_MSEC,   NULL,   NULL},
    { enable,          JOB_STARTED_EVENT,          UNEXPECT_SKIP_OTHER,   TEN_MSEC,   NULL,   NULL},
    { conv,            JOB_FINISHED_EVENT,         EXPECT_SKIP_OTHER,     TEN_SEC,    NULL,   NULL},
    { quit,            FINI_EVENT,                 EXPECT_SKIP_OTHER,     TEN_MSEC,   NULL,   NULL},
    { NULL,            EOF_EVENT,                  0,                     TEN_MSEC,   NULL,   NULL}
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini=test_teardown, .timeout=20){
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);

    assert_valid_conversion("spool/Alice_bbb_*", "aaa", "bbb");

    assert_proper_exit_status(err, status);
}

#undef TEST_NAME
#undef type1
#undef type2
#undef printer
#undef job
#undef enable
#undef conv
#undef quit
