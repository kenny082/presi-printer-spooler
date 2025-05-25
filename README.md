# Presi Printer Spooler

A low-level UNIX print spooler implemented in **C**, simulating real-world job orchestration using **process control**, **signals**, and **conversion pipelines**.

## Features

- Interactive command-line interface to control job spooling, status, pausing/resuming, and canceling
- Dynamic file type registration and printer assignment
- Real-time job scheduling with signal-safe inter-process communication
- Conversion pipelines built using `fork()`, `execvp()`, `dup2()`, and `pipe()`
- Robust job lifecycle management: created, running, paused, finished, aborted, deleted
- Prompt job dispatching with sub-10ms latency

## Technologies Used

- **C (C99)**
- **UNIX processes**, **signals**
- **Inter-process pipes**
- **POSIX-compliant system calls**
- **Custom CLI** (interactive & batch)

## Directory Overview

```
src/
├── main.c       # Entry point (do not modify)
├── cli.c        # Command parser and spooler logic
include/
├── presi.h, conversions.h, sf_readline.h
util/
├── printer, convert
├── stop_printers.sh, show_printers.sh
tests/
├── basic_tests.c, job_tests.c, printer_tests.c
```

## Building

```bash
make all        # Builds all binaries
make clean      # Cleans compiled files
make debug      # Builds with debug symbols
```

## Running

### Interactive Mode

```bash
bin/presi
```

### Batch Mode with Commands

```bash
bin/presi -i commands.txt
```

### Output to File

```bash
bin/presi -i commands.txt -o output.txt
```
## Testing Instructions

1. Compile the project:

    ```bash
    make all
    ```

2. Run the test suite:

    ```bash
    bin/presi_tests -j1 --verbose
    ```


## Sample Commands

```text
type pdf
type ps
printer Alice ps
conversion pdf ps pdf2ps - -
print document.pdf
jobs
pause 0
resume 0
cancel 0
quit
```
