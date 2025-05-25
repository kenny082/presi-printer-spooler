#ifndef SF_EVENT_H
#define SF_EVENT_H

#include "presi.h"

#define EVENT_SOCKET "event_socket"
#define MSG_MAX 128
#define NAME_MAX 32
#define TYPE_MAX 16
#define CMD_MAX 128
#define PATH_MAX 16

typedef enum event_type {
    NO_EVENT, ANY_EVENT, EOF_EVENT,   // Reserved meta-events, used by tracker.
    INIT_EVENT,                       // Signals program startup.
    FINI_EVENT,                       // Signals program shutdown.

    CMD_OK_EVENT,                     // Indicates successful command.
    CMD_ERROR_EVENT,                  // Indicates command error.

    TYPE_DEFINED_EVENT,               // File type defined.
    CONVERSION_DEFINED_EVENT,         // Conversion defined.

    PRINTER_DEFINED_EVENT,	          // Printer defined.
    PRINTER_STATUS_EVENT,             // Printer status report.

    JOB_CREATED_EVENT,                // Signals job being created.
    JOB_STARTED_EVENT,                // Signals job starting.
    JOB_FINISHED_EVENT,               // Signals job terminating normally.
    JOB_ABORTED_EVENT,                // Signals job terminating abnormally.
    JOB_DELETED_EVENT,                // Signals terminated job being deleted.
    JOB_STATUS_EVENT,                 // Job status report.
} EVENT_TYPE;

typedef struct event {
    int type;
    struct timeval time;
    char msg[MSG_MAX];
    char printer_name[NAME_MAX];
    PRINTER_STATUS printer_status;
    int jobid;
    int pgid;
    int exit_status;
    int term_signal;
    JOB_STATUS job_status;
    char file_type[TYPE_MAX];
    char new_type[TYPE_MAX];
    char conv_cmd[CMD_MAX];
    char path[PATH_MAX][TYPE_MAX];
} EVENT;

typedef struct event EVENT;

char *event_type_name(EVENT_TYPE type);

#endif
