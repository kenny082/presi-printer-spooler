#ifndef PRESI_H
#define PRESI_H

#include <stdint.h>
#include <stdlib.h>

/* PRINTERS */

#define MAX_PRINTERS 32       /* Maximum number of printers. */

/*
 * Structure that describes a printer.
 * You must define this appropriately (but not here).
 */
typedef struct printer PRINTER;

/*
 * Possible statuses for a printer.  See the assignment document for more information.
 */
typedef enum {
    PRINTER_DISABLED,
    PRINTER_IDLE,
    PRINTER_BUSY
} PRINTER_STATUS;

extern char *printer_status_names[];

/*
 * Function to connect to the specified printer, which is started if it
 * is not already.  Flags modify behavior of printer on startup -- if you want to
 * change the behavior of an already running printer, it is necessary to kill that
 * printer first, so that it gets restarted the next time you try to connect to it.
 *
 * @param printer_name  The name of the printer, as declared with the "printer" command.
 * @param printer_type  The type of file the printer can print, as declared with the
 * "printer command".
 * @param flags  Bitmap of flags that modify the behavior of the printer in
 * various ways, for debugging purposes.
 * @return the file descriptor to be used to write to printer, if successful,
 * otherwise -1 if unsuccessful.
 */
int presi_connect_to_printer(char *printer_name, char *printer_type, int flags);

/* Flags for presi_connect_to_printer() */
#define PRINTER_NORMAL (0x0)  /* "Normal" printer behavior. */
#define PRINTER_DELAYS (0x1)  /* Printing may involve random delays. */
#define PRINTER_FLAKY (0x2)   /* Printer may randomly "disconnect". */

/* JOBS */

#define MAX_JOBS 64       /* Maximum number of jobs. */

/*
 * Structure that describes a job.
 * You must define this appropriately (but not here).
 */
typedef struct job JOB;

/*
 * Possible statuses for a job.  See the assignment document for more information.
 */
typedef enum {
    JOB_CREATED,
    JOB_RUNNING,
    JOB_PAUSED,
    JOB_FINISHED,
    JOB_ABORTED,
    JOB_DELETED
} JOB_STATUS;

extern char *job_status_names[];

/*
 * FUNCTIONS YOU MUST IMPLEMENT
 * See the assignment document for further information.
 */

int run_cli(FILE *in, FILE *out);

/*
 * EVENT FUNCTIONS THAT YOU MUST CALL AT SPECIFIED TIMES
 * See the assignment document for further information.
 */

void sf_cmd_ok(void);
void sf_cmd_error(char *msg);

void sf_printer_defined(char *name, char *type);
void sf_printer_status(char *name, PRINTER_STATUS status);

void sf_job_created(int id, char *file_name, char *file_type);
void sf_job_started(int id, char *printer, int pgid, char **path);
void sf_job_finished(int id, int status);
void sf_job_aborted(int id, int status);
void sf_job_deleted(int id);
void sf_job_status(int id, JOB_STATUS status);

/*
 * EVENT FUNCTIONS THAT ARE ALREADY CALLED FOR YOU
 */

// Called from main()
void sf_init(void);
void sf_fini(void);

// Called from conversions.o
void sf_type_defined(char *name);
void sf_conversion_defined(char *from, char *to, char **cmd_and_args);

#endif
