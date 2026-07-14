#ifndef SCHEDULE_PARSE_H
#define SCHEDULE_PARSE_H

#include <stdio.h>
#include <stdbool.h>

// Streaming converter: EMF Camp schedule feed (a large JSON array of event
// objects) -> the badge schedule format, one row written to an output FILE*
// as each event is recognised. Designed to be fed arbitrary byte fragments
// (e.g. HTTP body chunks) and to keep RAM bounded regardless of feed size:
// only one event object at a time is buffered, and the large free-text
// fields (description/short_description) are stripped as they stream by.
//
// This module is host-portable (only stdio + cJSON), so the state machine can
// be unit-tested off-device.

#define SP_SLIM_CAP 2048   // max buffered size of one (stripped) event object
#define SP_KEY_CAP  40     // max tracked top-level key length

typedef enum {
    SP_PRE_ARRAY,  // before the top-level '['
    SP_IN_ARRAY,   // between objects
    SP_IN_OBJECT,  // inside one event object
    SP_DONE,       // saw the closing ']'
    SP_ERR         // unrecoverable (write failure); framing aborted
} sp_phase_t;

// Object sub-states (only meaningful while phase == SP_IN_OBJECT)
typedef enum {
    OS_MEMBERS,      // expecting a key '"' or object end '}'
    OS_KEY,          // reading a top-level key
    OS_COLON,        // after key, expecting ':'
    OS_VALUE_START,  // after ':', expecting first char of the value
    OS_VALUE,        // consuming the value (scalar or structured)
    OS_AFTER_VALUE   // after a value, expecting ',' or '}'
} sp_ostate_t;

typedef struct {
    sp_phase_t phase;
    FILE* out;             // destination for emitted rows (rows only, no framing)
    int   day1_dom;        // day-of-month mapped to "day 1" (EMF 2026: 16)
    bool  first_row;       // for comma separation between emitted rows
    int   rows;            // emitted row count
    bool  write_error;     // a fwrite/format failed

    // low-level tokenizer (whole object)
    int  depth;            // nesting depth inside the object (1 = top members)
    bool in_string;
    bool escape;

    // object slim-copy buffer (object text minus the big free-text values)
    char slim[SP_SLIM_CAP];
    int  slim_len;
    bool overflow;         // object exceeded SP_SLIM_CAP -> skip it

    // member sub-state
    sp_ostate_t os;
    char key[SP_KEY_CAP];
    int  key_len;
    bool suppress;         // currently skipping a stripped string value's body
    bool skip_value;       // the current member's value should be blanked
} sp_state_t;

// Initialise for a fresh stream. out receives only the row objects (the caller
// writes the surrounding {"info":...,"schedule":[  ... ]} framing).
void sp_init(sp_state_t* s, FILE* out, int day1_dom);

// Feed an arbitrary span of feed bytes. Safe across any chunk boundary.
void sp_feed(sp_state_t* s, const char* buf, int len);

// True if the array was closed cleanly and no write error occurred.
bool sp_ok(const sp_state_t* s);

#endif // SCHEDULE_PARSE_H
