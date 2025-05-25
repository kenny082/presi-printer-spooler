/*
 * Presi: Command-line interface
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "presi.h"
#include "conversions.h"
#include "sf_readline.h"

#include <time.h> // for job creation time and last update
#include <unistd.h> // fork() and execvp(), dup,dup2
#include <fcntl.h> // for open, since we are not reading from FILE* streams but UNIX FD 
#include <sys/wait.h> // wait
#include <signal.h> // signals

#define JOB_DELETION_DELAY 10 // seconds

struct printer {
    int id; // unique ID from 0 to MAX_PRINTERS - 1
    char *name; // any name
    char *type; // pdf, ps, pbm, ascii, etc
    PRINTER_STATUS status; // DISABLED, IDLE, BUSY
};

typedef struct job{
    int id; // unique job ID; any naming scheme
    char *filename; // file to be "printed"
    char *file_type; // pdf, ps, pbm, ascii, etc
    int status; // CREATED, RUNNING, PAUSED, FINISHED, ABORTED, DELETED
    PRINTER *assigned_printer; // pointer to actual printer used for job once started
    pid_t pgid; // track process group ID for conversion pipelines
    pid_t pid; // PID of the main pipeline process
    int eligible_bitmap; // bitmask of eligible set of printers for specific job (0-31)
    int marked_for_deletion; // mark a job for deletion
    time_t creation_time; // track time of job creation
    time_t status_time; // track last status change
} JOB;

// Global variables just to keep track of printer and jobs created along with a list storing them
PRINTER *printers[MAX_PRINTERS] = {NULL};
int printer_count = 0;
JOB *jobs[MAX_JOBS] = {NULL};
JOB *job_queue[MAX_JOBS] = {NULL}; 
int queue_length = 0;

volatile sig_atomic_t sigchld_received = 0;
static int next_job_id = 0;

// Function prototypes
void try_dispatch_jobs(FILE *out);
pid_t start_pipeline(JOB *job, PRINTER *printer, CONVERSION **path, int path_len);
void sigchld_handler(int signo);
void check_child_statuses(FILE *out);
void process_signals(void);
void delete_old_jobs(void);
void free_deleted_jobs(void);
void block_signal(int sig);
void unblock_signal(int sig);

void block_signal(int sig) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void unblock_signal(int sig) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void try_dispatch_jobs(FILE *out) {
    block_signal(SIGCHLD);
    for (int i = 0; i < queue_length; i++) {
        JOB *job = job_queue[i];
        if (!job || job->status != JOB_CREATED) continue;
        for (int j = 0; j < printer_count; j++) {
            PRINTER *printer = printers[j];
            if (printer->status != PRINTER_IDLE) continue;
            if (!(job->eligible_bitmap & (1 << printer->id))) continue;
            CONVERSION **path = find_conversion_path(job->file_type, printer->type);
            if (!path) continue;
            int path_len = 0;
            while (path[path_len]) path_len++;
            pid_t pgid = start_pipeline(job, printer, path, path_len);
            if (pgid < 0) {
                sf_cmd_error("Pipeline failed to start");
                free(path);
                return;
            }
            job->pgid = pgid;
            job->pid = pgid;
            job->assigned_printer = printer;
            job->status = JOB_RUNNING;
            job->status_time = time(NULL);
            printer->status = PRINTER_BUSY;
            sf_job_status(job->id, JOB_RUNNING);
            sf_printer_status(printer->name, PRINTER_BUSY);
            char **command_names = malloc((path_len + 1) * sizeof(char *));
            for (int k = 0; k < path_len; k++) {
                command_names[k] = path[k]->cmd_and_args[0];
            }
            command_names[path_len] = NULL;
            sf_job_started(job->id, printer->name, pgid, command_names);
            free(command_names);
            free(path);
            break; // job has been started
        }
    }
    unblock_signal(SIGCHLD);
}

pid_t start_pipeline(JOB *job, PRINTER *printer, CONVERSION **path, int path_len) {
    pid_t master_pid = fork();
    if (master_pid < 0) {
        return -1;
    }
    if (master_pid == 0) {
        // Child: pipeline master
        setpgid(0, 0);
        // Zero-conversion case: wire stdin->file, stdout->printer, then cat
        if (path_len == 0) {
            // open the input file for stdin
            int in_fd = open(job->filename, O_RDONLY);
            if (in_fd < 0) {
                exit(EXIT_FAILURE);
            }
            if (dup2(in_fd, STDIN_FILENO) < 0) {
                exit(EXIT_FAILURE);
            }
            close(in_fd);
            // connect stdout to the printer daemon
            int printer_fd = presi_connect_to_printer(printer->name,printer->type,PRINTER_NORMAL);
            if (printer_fd < 0) {
                exit(EXIT_FAILURE);
            }
            if (dup2(printer_fd, STDOUT_FILENO) < 0) {
                exit(EXIT_FAILURE);
            }
            close(printer_fd);
            // exec cat to copy stdin->stdout (file->printer)
            char *cat_args[] = { "/bin/cat", NULL };
            execvp(cat_args[0], cat_args);
            exit(EXIT_FAILURE);
        }
        // Build N‑stage conversion pipeline
        int pipes[path_len][2];
        for (int i = 0; i < path_len; i++) {
            if (pipe(pipes[i]) == -1) {
                exit(EXIT_FAILURE);
            }
        }
        for (int i = 0; i < path_len; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                // Child stage
                setpgid(0, 0);
                // stdin for this stage
                if (i == 0) {
                    int in_fd = open(job->filename, O_RDONLY);
                    if (in_fd < 0) {
                        exit(EXIT_FAILURE);
                    }
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                } else {
                    dup2(pipes[i-1][0], STDIN_FILENO);
                }
                // stdout for this stage
                if (i == path_len - 1) {
                    int printer_fd = presi_connect_to_printer(printer->name,printer->type,PRINTER_NORMAL);
                    if (printer_fd < 0) {
                        exit(EXIT_FAILURE);
                    }
                    dup2(printer_fd, STDOUT_FILENO);
                    close(printer_fd);
                } else {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }
                // close all pipe fds
                for (int j = 0; j < path_len; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                // exec conversion command
                execvp(path[i]->cmd_and_args[0], path[i]->cmd_and_args);
                exit(EXIT_FAILURE);
            }
            // Parent closes unused ends
            if (i > 0) close(pipes[i-1][0]);
            if (i < path_len - 1) close(pipes[i][1]);
        }
        // Master waits for all children
        int status, failed = 0;
        while (wait(&status) > 0) {
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                failed = 1;
            }
        }
        exit(failed);
    }
    return master_pid; // Parent (main presi) tracks master PID as pgid
}

void sigchld_handler(int signo) {
    sigchld_received = 1;
}

void check_child_statuses(FILE *out) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            JOB *job = jobs[i];
            if (!job || job->pgid == -1 || job->pid != pid) continue;
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                job->status = JOB_FINISHED;
                job->status_time = time(NULL);
                sf_job_status(job->id, JOB_FINISHED);
                sf_job_finished(job->id, status);
                job->pgid = -1;
                job->pid = -1;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->status = JOB_ABORTED;
                job->status_time = time(NULL);
                sf_job_status(job->id, JOB_ABORTED);
                sf_job_aborted(job->id, status);
                job->pgid = -1;
                job->pid = -1;
            } else if (WIFSTOPPED(status)) {
                job->status = JOB_PAUSED;
                job->status_time = time(NULL);
                sf_job_status(job->id, JOB_PAUSED);
            } else if (WIFCONTINUED(status)) {
                job->status = JOB_RUNNING;
                job->status_time = time(NULL);
                sf_job_status(job->id, JOB_RUNNING);
            }
            if ((WIFEXITED(status) || WIFSIGNALED(status)) && job->assigned_printer) {
                job->assigned_printer->status = PRINTER_IDLE;
                sf_printer_status(job->assigned_printer->name, PRINTER_IDLE);
                try_dispatch_jobs(out); // try the next job
            }
            break; // found matching job
        }
    }
}

void process_signals(void) {
    while(sigchld_received) {
        sigchld_received = 0;
        check_child_statuses(stdout);
    }
}

void delete_old_jobs(void) {
    block_signal(SIGCHLD);
    time_t now = time(NULL);
    for (int i = 0; i < MAX_JOBS; i++) {
        JOB *job = jobs[i];
        if (!job || job->marked_for_deletion) continue;
        if ((job->status != JOB_FINISHED && job->status != JOB_ABORTED))
            continue;
        if ((now - job->status_time) >= JOB_DELETION_DELAY) {
            job->marked_for_deletion = 1;
            sf_job_status(job->id, JOB_DELETED);
            sf_job_deleted(job->id);
        }
    }
    unblock_signal(SIGCHLD);
}

void free_deleted_jobs(void) {
    block_signal(SIGCHLD);
    int deleted_count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        JOB *job = jobs[i];
        if (job && job->marked_for_deletion) {
            free(job->filename);
            free(job->file_type);
            free(job);
            jobs[i] = NULL;
            deleted_count++;
        }
    }
    int new_len = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_queue[i] && !job_queue[i]->marked_for_deletion) {
            job_queue[new_len++] = job_queue[i];
        }
    }
    for (int i = new_len; i < MAX_JOBS; i++) {
        job_queue[i] = NULL;
    }
    queue_length = new_len;
    unblock_signal(SIGCHLD);
}

int run_cli(FILE *in, FILE *out){
    static int initialized = 0;
    if(!initialized){
        struct sigaction sa;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if(sigaction(SIGCHLD, &sa, NULL) == -1){
            exit(EXIT_FAILURE);
        }
        sf_set_readline_signal_hook(process_signals);
        initialized = 1;
    }
    char *line;
    int quit_flag = 0;
    // Setup batchmode
    int redirected = 0;
    int saved_stdin = -1;
    if(in != NULL && in != stdin){ // redirect stdin to read from batch files
        redirected = 1;
        saved_stdin = dup(STDIN_FILENO);
        dup2(fileno(in), STDIN_FILENO);
    }
    while(!quit_flag){
        if (in == stdin && out == stdout){  
            line = sf_readline("presi> ");
        }
        else{
            line = sf_readline(NULL);
        }
        process_signals();
        if (line == NULL) {
            if (redirected){
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (in == NULL || in == stdin){
                return -1;
            }
            else{
                return 0;
            }
        }
        delete_old_jobs();
        free_deleted_jobs();
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        // Commands consist of sequence of words w/ white spaces between - tokenize (used from lecture slides: " \t\n\r")
        char **tokens = NULL; // creating an array of strings that will store our tokens from sf_readline
        int token_count = 0;
        int token_capacity = 8;
        tokens = malloc(token_capacity * sizeof(char *));
        if(!tokens){
            free(line);
            return -1;
        }
        char *saveptr;
        char *token = strtok_r(line, " \t\n\r", &saveptr); // Parse based on delimitter
        while(token){
            if (token_count >= token_capacity){
                token_capacity <<= 1;
                char **new_tokens = realloc(tokens, token_capacity * sizeof(char*));
                if (!new_tokens){
                    free(tokens);
                    free(line);
                    return -1;
                }
                tokens = new_tokens;
            }
            tokens[token_count++] = token; // stores token pointer into array
            token = strtok_r(NULL, " \t\n\r", &saveptr); // gets the next token
        }
        tokens = realloc(tokens, (token_count + 1) * sizeof(char*));
        tokens[token_count] = NULL;
        if (token_count == 0){
            free(tokens);
            free(line);
            continue;
        }
        if (strcmp(tokens[0], "help") == 0) {
            if (token_count != 1){
                fprintf(out, "Wrong number of args (given: %d, required: 0) for CLI command 'help\n", token_count - 1);
                sf_cmd_error("Help requires no parameters with format: help");
            }
            else{
                fprintf(out, "Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume\n");
                sf_cmd_ok();
            }
        } else if (strcmp(tokens[0], "quit") == 0) {
            if(token_count != 1){
                fprintf(out, "Wrong numbers of args (given: %d, required: 0) for CLI command 'quit'\n", token_count - 1);
                sf_cmd_error("Quit requires no parameters with format: quit");
            }
            else{
                block_signal(SIGCHLD);
                for (int i = 0; i < MAX_JOBS; i++) {
                    JOB *job = jobs[i];
                    if (!job) continue;
                    if (job->status == JOB_RUNNING || job->status == JOB_PAUSED) {
                        killpg(job->pgid, SIGTERM);
                        if (job->status == JOB_PAUSED) {
                            killpg(job->pgid, SIGCONT);
                        }
                        int status;
                        waitpid(job->pgid, &status, 0); 
                        job->pgid = -1;
                        job->pid = -1;
                        job->status = JOB_ABORTED;
                        job->status_time = time(NULL);
                        sf_job_status(job->id, JOB_ABORTED);
                        sf_job_aborted(job->id, status);
                        if (job->assigned_printer) {
                            job->assigned_printer->status = PRINTER_IDLE;
                            sf_printer_status(job->assigned_printer->name, PRINTER_IDLE);
                        }
                    }
                    free(job->filename);
                    free(job->file_type);
                    free(job);
                    jobs[i] = NULL;
                }
                unblock_signal(SIGCHLD);
                sigchld_received = 0;
                sf_cmd_ok();
                free(tokens);
                free(line);
                return -1;
            }
        } else if (strcmp(tokens[0], "type") == 0) {
            if(token_count != 2){
                fprintf(out, "Wrong number of args (given: %d, required: 1) for CLI command 'type'\n", token_count - 1);
                sf_cmd_error("Type requires two parameters with format: type file_type");
            }
            else if (find_type(tokens[1]) != NULL){
                sf_cmd_error("File type already defined"); // Multiple same type does not make sense
            }
            else{
                FILE_TYPE *type = define_type(tokens[1]);
                if (type == NULL){
                    sf_cmd_error("Could not define this file type");
                }
                else{
                    sf_cmd_ok();
                }
            }
        } else if (strcmp(tokens[0], "printer") == 0) {
            if(token_count != 3){
                fprintf(out,"Wrong number of args (given: %d, required: 2) for CLI command 'printer'\n", token_count - 1);
                sf_cmd_error("Print requires three parameters: printer printer_name file_type");
            }
            else{
                char *printer_name = tokens[1]; // printer printer_name file_type
                char *file_type_name = tokens[2];
                FILE_TYPE *type = find_type(file_type_name);
                if (type == NULL){
                    sf_cmd_error("The type associated with the printer has not been created");
                    fprintf(out,"Command error: printer (failed)\n");
                }
                else{
                    int duplicate = 0;
                    for (int i = 0; i < printer_count; i++){
                        if (printers[i] && strcmp(printers[i]->name, printer_name) == 0) {
                            duplicate = 1;
                            break;
                        }
                    }
                    if (duplicate){
                        sf_cmd_error("Printer already defined"); // Does not make sense to have two of same printer
                    }
                    else if(printer_count >= MAX_PRINTERS){
                        sf_cmd_error("Maximum number of printers reached");
                    }
                    else{
                        PRINTER *printer = malloc(sizeof(PRINTER));
                        if(!printer){
                            sf_cmd_error("Memory allocation failed for new printer");
                        }
                        else{
                            // String duplicate, duplicates the input string which is pointer to line buffer which is later freed
                            printer->id = printer_count;
                            printer->name = strdup(printer_name);
                            printer->type = strdup(file_type_name);
                            printer->status = PRINTER_DISABLED;
                            printers[printer_count++] = printer;
                            sf_printer_defined(printer_name,file_type_name);
                            fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n", printer->id, printer->name, printer->type, printer_status_names[printer->status]);
                            sf_cmd_ok();
                        }
                    }
                }
            }
        } else if (strcmp(tokens[0], "conversion") == 0) {
            if (token_count < 4){
                fprintf(out, "Wrong number of args (given: %d, required: at least 3) for CLI command 'conversion'\n", token_count - 1);
                sf_cmd_error("Conversion command error");
            }
            else{
                char *from_type_name = tokens[1];
                char *to_type_name = tokens[2];
                FILE_TYPE *from_type = find_type(from_type_name);
                FILE_TYPE *to_type = find_type(to_type_name);
                if(!from_type){
                    fprintf(out, "Undeclared file type: %s\n", from_type_name);
                    sf_cmd_error("Command error, undeclared first file type (conversion failed)");
                }
                else if (!to_type){
                    fprintf(out, "Undeclared file type: %s\n", to_type_name);
                    sf_cmd_error("Command error, undeclared second file type (conversion failed)");
                }
                else{
                    int argc = token_count - 3; // Parameters + additional arguments (0 - arbitrary large)
                    char **cmd_and_args = malloc((argc + 1) * sizeof(char *));
                    if (!cmd_and_args) {
                        sf_cmd_error("Memory allocation failed");
                        return -1;
                    }
                    for (int i = 0; i < argc; i++) {
                        cmd_and_args[i] = tokens[3 + i];
                    }
                    cmd_and_args[argc] = NULL;
                    CONVERSION *conv = define_conversion(from_type_name, to_type_name, cmd_and_args);
                    if(!conv){
                        sf_cmd_error("Failed to define the conversion / could not find a path");
                    }
                    else{
                        sf_cmd_ok();
                        try_dispatch_jobs(out);
                    }
                    free(cmd_and_args);
                }
            }
        } else if (strcmp(tokens[0], "printers") == 0) {
            if(token_count != 1){
                fprintf(out,"Wrong number of args (given: %d, required: 0) for CLI command 'jobs'\n", token_count - 1);
                sf_cmd_error("Jobs requires one parameter: jobs");
            }
            else{
                for (int i = 0; i < printer_count; i++){
                    PRINTER *p = printers[i];
                    if(p){
                        fprintf(out,"PRINTER: id=%d, name=%s, type=%s, status=%s\n",
                        p->id, p->name, p->type, printer_status_names[p->status]);
                    }
                }
                sf_cmd_ok();
            }
        } else if (strcmp(tokens[0], "jobs") == 0) {
            if(token_count != 1){
                fprintf(out,"Wrong number of args (given: %d, required: 0) for CLI command 'jobs'\n", token_count - 1);
                sf_cmd_error("Jobs requires one parameter: jobs");
            }
            else{
                for (int i = 0; i < queue_length; i++){
                    JOB *job = job_queue[i];
                    if(!job) continue;
                    char created_buf[64], status_buf[64];
                    strftime(created_buf, sizeof(created_buf), "%d %b %H:%M:%S", localtime(&job->creation_time));
                    strftime(status_buf, sizeof(status_buf), "%d %b %H:%M:%S", localtime(&job->status_time));
                    fprintf(out, "JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%08x, file=%s\n",
                            job->id,job->file_type,created_buf,status_buf,job_status_names[job->status],job->eligible_bitmap,job->filename);
                }
                sf_cmd_ok();
            }
        } else if (strcmp(tokens[0], "print") == 0) {
            if (token_count < 2){
                fprintf(out, "Wrong number of args (given: %d, required: 1) for CLI command 'print'\n", token_count - 1);
                sf_cmd_error("Print command requires format: print file_name.file_type");
            }
            else{
                char *filename = tokens[1];
                FILE_TYPE *ft = infer_file_type(filename);
                if(!ft){
                    sf_cmd_error("Print command requires that the file type is defined previously)");
                    fprintf(out, "Command error: print (file type)\n");
                }
                else{
                    int id = -1;
                    int start = next_job_id;
                    do{
                        if(jobs[next_job_id] == NULL){
                            id = next_job_id;
                            next_job_id = (next_job_id + 1) % MAX_JOBS;
                            break;
                        }
                        next_job_id = (next_job_id + 1) % MAX_JOBS;
                    } while (next_job_id != start);
                    if (id == -1){
                        sf_cmd_error("Maximum jobs reached");
                        fprintf(out,"Command error: print (failed)\n");
                    }
                    else{
                        JOB *job = malloc(sizeof(JOB));
                        if(!job){
                            sf_cmd_error("Allocation failed for job");
                            return 0;
                        }
                        job->marked_for_deletion = 0;
                        job->id = id;
                        job->pid = -1;
                        job->filename = strdup(filename);
                        job->file_type = strdup(ft->name);
                        job->assigned_printer = NULL;
                        job->pgid = -1;
                        job->status = JOB_CREATED;
                        time_t now = time(NULL);
                        job->creation_time = now;
                        job->status_time = 0;
                        // Setting eligible printer bitmap
                        int bitmap = 0;
                        int failed_flag = 0;
                        if(token_count == 2){ // Meaning no additional arguments, all 32 printers are available
                            bitmap = 0xFFFFFFFF; // Set all eligibile by default due to future eligiblity
                        }
                        else{
                            for (int i = 2; i < token_count; i++){  // Loop through specified printers
                                int found = 0;
                                for (int j = 0; j < printer_count; j++){ // Loop through all printers
                                    if(strcmp(tokens[i], printers[j]->name) == 0){ // Check if specified = any printer
                                        bitmap |= (1 << printers[j]->id); // Set availability if equal
                                        found = 1;
                                        break; // Since printers are unique
                                    }
                                }
                                if(!found){
                                    fprintf(out, "Unknown printer name: '%s'\n", tokens[i]);
                                    sf_cmd_error("One or more specified printers are not declared");
                                    failed_flag = 1;
                                    break;
                                }
                            }
                            if (bitmap == 0 && failed_flag == 0){
                                fprintf(out, "No eligible printer currently for job, will wait incase of future conversions");
                            }
                        }
                        if(failed_flag){
                            free(job->filename);
                            free(job->file_type);
                            free(job);
                        }
                        else{
                            job->eligible_bitmap = bitmap;
                            block_signal(SIGCHLD);
                            jobs[id] = job;
                            job_queue[queue_length++] = job;
                            job->id = id;
                            char created_buf[64], status_buf[64];
                            strftime(created_buf, sizeof(created_buf), "%d %b %H:%M:%S", localtime(&job->creation_time));
                            strftime(status_buf, sizeof(status_buf), "%d %b %H:%M:%S", localtime(&job->status_time));
                            unblock_signal(SIGCHLD);
                            sf_job_created(job->id, job->filename, job->file_type);
                            fprintf(out, "JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%08x, file=%s\n",
                                job->id,job->file_type,created_buf,status_buf,job_status_names[job->status],job->eligible_bitmap,job->filename);
                            try_dispatch_jobs(out);
                            sf_cmd_ok();
                        }
                    }
                }
            }
        } else if (strcmp(tokens[0], "cancel") == 0) {
            if (token_count != 2){
                fprintf(out, "Wrong number of args (given: %d, required: 1) for CLI command 'cancel'\n", token_count - 1);
                sf_cmd_error("Cancel command requires format: cancel job_number");
            }
            else{
                char *endptr;
                long id_long = strtol(tokens[1], &endptr, 10);
                if(*endptr != '\0' || id_long < 0 || id_long >= MAX_JOBS || !jobs[id_long]){
                    sf_cmd_error("Invalid job ID");
                }
                else{
                    int id = (int)id_long;
                    JOB *job = jobs[id];
                    if (job->status != JOB_CREATED && job->status != JOB_RUNNING && job->status != JOB_PAUSED) {
                        sf_cmd_error("Job cannot be canceled in its current state");
                    } else {
                        block_signal(SIGCHLD);
                        if (job->status == JOB_RUNNING || job->status == JOB_PAUSED) {
                            killpg(job->pgid, SIGTERM);
                            if (job->status == JOB_PAUSED) {
                                killpg(job->pgid, SIGCONT);
                            }
                        }
                        job->status = JOB_ABORTED;
                        job->status_time = time(NULL);
                        sf_job_status(job->id, JOB_ABORTED);
                        sf_job_aborted(job->id, job->pgid);
                        if (job->assigned_printer) {
                            job->assigned_printer->status = PRINTER_IDLE;
                            sf_printer_status(job->assigned_printer->name, PRINTER_IDLE);
                        }
                        unblock_signal(SIGCHLD);
                        sf_cmd_ok();
                    }
                }
            }
        } else if (strcmp(tokens[0], "pause") == 0) {
            if (token_count != 2){
                fprintf(out, "Wrong number of args (given: %d, required: 1) for CLI command 'pause'\n", token_count - 1);
                sf_cmd_error("Pause command requires format: pause job_number");
            }
            else{
                char *endptr;
                long id_long = strtol(tokens[1], &endptr, 10);
                if (*endptr != '\0' || id_long < 0 || id_long >= MAX_JOBS || !jobs[id_long]) {
                    sf_cmd_error("Invalid job ID");
                } else {
                    int id = (int)id_long;
                    JOB *job = jobs[id];
                    if (job->status != JOB_RUNNING) {
                        sf_cmd_error("Job is not running");
                    } else {
                        block_signal(SIGCHLD);
                        if (killpg(job->pgid, SIGSTOP) == -1) {
                            unblock_signal(SIGCHLD);
                            sf_cmd_error("Failed to pause job");
                        } else {
                            unblock_signal(SIGCHLD);
                            sf_cmd_ok();
                        }
                    }
                }
            }
        } else if (strcmp(tokens[0], "resume") == 0) {
            if (token_count != 2) {
                sf_cmd_error("Usage: resume <job_id>");
            } else {
                char *end;
                long id_long = strtol(tokens[1], &end, 10);
                if (*end != '\0' || id_long < 0 || id_long >= MAX_JOBS || !jobs[id_long]) {
                    sf_cmd_error("Invalid job ID");
                } else {
                    int id = (int)id_long;
                    JOB *job = jobs[id];
                    if (job->status != JOB_PAUSED) {
                        sf_cmd_error("Job is not paused");
                    } else {
                        block_signal(SIGCHLD);
                        if (killpg(job->pgid, SIGCONT) == -1) {
                            unblock_signal(SIGCHLD);
                            sf_cmd_error("Failed to resume job");
                        } else {
                            unblock_signal(SIGCHLD);
                            sf_cmd_ok();
                        }
                    }
                }
            }
        } else if (strcmp(tokens[0], "disable") == 0) {
            if (token_count != 2){
                sf_cmd_error("Printer disable requires format of : disable name_of_existing_printer");
            }
            else{
                char *target_name = tokens[1];
                PRINTER *printer = NULL;
                for (int i = 0; i < printer_count; i++){
                    if (printers[i] && strcmp(printers[i]->name, target_name) == 0){
                        printer = printers[i];
                        break;
                    }
                }
                if(!printer){
                    sf_cmd_error("Printer to disable was not found");
                    fprintf(out,"Command error: disable (no printer)\n");
                }
                else{
                    printer->status = PRINTER_DISABLED;
                    sf_printer_status(printer->name, printer->status);
                    fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n",
                        printer->id, printer->name, printer->type,
                        printer_status_names[printer->status]);
                    sf_cmd_ok();
                }
            }
        } else if (strcmp(tokens[0], "enable") == 0) {
            if (token_count != 2){
                sf_cmd_error("Printer enable requires format of: enable name_of_existing_printer");
            }
            else{
                char *target_name = tokens[1];
                PRINTER *printer = NULL;
                for (int i = 0; i < printer_count; i++){
                    if (printers[i] && strcmp(printers[i]->name, target_name) == 0){
                        printer = printers[i];
                        break;
                    }
                }
                if(!printer){
                    sf_cmd_error("Printer to enable was not found");
                    fprintf(out,"Command error: enable (no printer)\n");
                }
                else{
                    printer->status = PRINTER_IDLE;
                    sf_printer_status(printer->name, printer->status);
                    fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n",
                        printer->id, printer->name, printer->type,
                        printer_status_names[printer->status]);
                    try_dispatch_jobs(out); // after enabling, immediately try processing
                    sf_cmd_ok();
                }
            }
        } else {
            sf_cmd_error("unrecognized command");
        }
        free(tokens);
        free(line);
    }
    if (redirected){ // restore stdin afterwards
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    return 0;
}