/*
 * Commands for test scripts.
 */

#include "../lib/sf_event.h"
typedef void (*ACTION)(EVENT *ep, int *env, void *args);

typedef struct command {
    char *send;
    EVENT_TYPE expect;
    int modifiers;
    struct timeval timeout;
    ACTION before;
    ACTION after;
    void *args;
} COMMAND;

/*
 * Modifiers for expect field.
 * The presence of any of these bits in the modifiers field changes
 * the way the expect is handled.
 */
#define EXPECT_SKIP_OTHER 0x1           // Skip non-matching events.

/*
 * Bogus modifiers for introducing send delays.
 * TODO (EWS): This should actually be done with a separate field in the script,
 * but I don't want to change all the existing tests right now.
 */
#define DELAY_1SEC 0x2
#define DELAY_15SEC 0x4

/*
 * Modifier for expect field.
 * If this modifier is set, then a matching event will abort, and all
 * non-matching events will be skipped. Essentially, it's the "opposite" of
 * the EXPECT_SKIP_OTHER command.
 */
#define UNEXPECT_SKIP_OTHER 0x8

/*
 * Predefined timeout values.
 */
#define ZERO_SEC { 0, 0 }
#define ONE_USEC { 0, 1 }
#define ONE_MSEC { 0, 1000 }
#define TEN_MSEC { 0, 10000 }
#define HND_MSEC { 0, 100000 }
#define ONE_SEC { 1, 0 }
#define TWO_SEC { 2, 0 }
#define THR_SEC { 3, 0 }
#define TEN_SEC { 10, 0 }
#define THTY_SEC { 30, 0 }
#define FFTY_SEC { 50, 0 }
#define HND_SEC { 100, 0 }

int run_test(char *name, char *target, char* av[], COMMAND *script, int *statusp);
