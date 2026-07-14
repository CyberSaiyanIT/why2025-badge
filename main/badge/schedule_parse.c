#include "schedule_parse.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cJSON.h"

// ---- helpers ---------------------------------------------------------------

static void sp_put(sp_state_t* s, char c)
{
    if (s->overflow) return;
    if (s->slim_len >= SP_SLIM_CAP - 1) {
        s->overflow = true;   // object too big; will be skipped
        return;
    }
    s->slim[s->slim_len++] = c;
}

// Parse "YYYY-MM-DD HH:MM:SS" -> fills fields; returns true on success.
static bool parse_dt(const char* s, int* y, int* mo, int* d, int* h, int* mi)
{
    int sec;
    if (!s) return false;
    return sscanf(s, "%d-%d-%d %d:%d:%d", y, mo, d, h, mi, &sec) == 6;
}

static const char* cj_str(const cJSON* o, const char* key)
{
    const cJSON* it = cJSON_GetObjectItem(o, key);
    if (cJSON_IsString(it) && it->valuestring) return it->valuestring;
    return "";
}

// Emit one badge row per occurrence of a talk. Writes to s->out.
static void sp_emit_from_slim(sp_state_t* s)
{
    if (s->overflow || s->slim_len == 0) return;
    s->slim[s->slim_len] = '\0';

    cJSON* ev = cJSON_Parse(s->slim);
    if (!ev) return;

    const char* type = cj_str(ev, "type");
    if (strcmp(type, "talk") != 0) { cJSON_Delete(ev); return; }

    const char* title   = cj_str(ev, "title");
    const char* speaker = cj_str(ev, "names");

    cJSON* occ = cJSON_GetObjectItem(ev, "occurrences");
    if (cJSON_IsArray(occ)) {
        cJSON* o = NULL;
        cJSON_ArrayForEach(o, occ) {
            int y, mo, d, h, mi;
            if (!parse_dt(cj_str(o, "start_date"), &y, &mo, &d, &h, &mi))
                continue;

            int day = d - (s->day1_dom - 1);
            const char* venue = cj_str(o, "venue");

            char hour[8];
            snprintf(hour, sizeof(hour), "%02d:%02d", h, mi);

            char sort[16];
            snprintf(sort, sizeof(sort), "%04d%02d%02d%02d%02d", y, mo, d, h, mi);

            char days[8];
            snprintf(days, sizeof(days), "%d", day);

            // duration (minutes), computed with day-of-month so it survives
            // events crossing midnight; empty when end is missing/invalid.
            char dur[16] = "";
            int ey, emo, ed, eh, emi;
            if (parse_dt(cj_str(o, "end_date"), &ey, &emo, &ed, &eh, &emi)) {
                long start_min = ((long)d * 24 + h) * 60 + mi;
                long end_min   = ((long)ed * 24 + eh) * 60 + emi;
                long diff = end_min - start_min;
                if (diff > 0 && diff < 100000) snprintf(dur, sizeof(dur), "%ld min", diff);
            }

            cJSON* row = cJSON_CreateObject();
            if (!row) continue;
            cJSON_AddStringToObject(row, "sort", sort);
            cJSON_AddStringToObject(row, "title", title);
            cJSON_AddStringToObject(row, "day", days);
            cJSON_AddStringToObject(row, "hour", hour);
            cJSON_AddStringToObject(row, "speaker", speaker);
            cJSON_AddStringToObject(row, "location", venue);
            cJSON_AddStringToObject(row, "duration", dur);

            char* txt = cJSON_PrintUnformatted(row);
            cJSON_Delete(row);
            if (!txt) continue;

            int ok = 1;
            if (!s->first_row) ok = (fputc(',', s->out) != EOF);
            if (ok) ok = (fputs(txt, s->out) != EOF);
            cJSON_free(txt);

            if (!ok) { s->write_error = true; cJSON_Delete(ev); return; }
            s->first_row = false;
            s->rows++;
        }
    }
    cJSON_Delete(ev);
}

static void sp_begin_object(sp_state_t* s)
{
    s->depth = 1;
    s->in_string = false;
    s->escape = false;
    s->slim_len = 0;
    s->overflow = false;
    s->os = OS_MEMBERS;
    s->key_len = 0;
    s->suppress = false;
    s->skip_value = false;
    s->phase = SP_IN_OBJECT;
    sp_put(s, '{');
}

static void sp_finalize_object(sp_state_t* s)
{
    sp_emit_from_slim(s);
    s->phase = (s->phase == SP_ERR) ? SP_ERR : SP_IN_ARRAY;
    if (s->write_error) s->phase = SP_ERR;
}

// ---- public API ------------------------------------------------------------

void sp_init(sp_state_t* s, FILE* out, int day1_dom)
{
    memset(s, 0, sizeof(*s));
    s->phase = SP_PRE_ARRAY;
    s->out = out;
    s->day1_dom = day1_dom;
    s->first_row = true;
}

bool sp_ok(const sp_state_t* s)
{
    return s->phase == SP_DONE && !s->write_error;
}

static void sp_feed_byte(sp_state_t* s, char c)
{
    unsigned char uc = (unsigned char)c;

    switch (s->phase) {
    case SP_PRE_ARRAY:
        if (c == '[') s->phase = SP_IN_ARRAY;
        return;

    case SP_IN_ARRAY:
        if (c == '{') { sp_begin_object(s); return; }
        if (c == ']') { s->phase = SP_DONE; return; }
        return;  // skip whitespace / commas between elements

    case SP_DONE:
    case SP_ERR:
        return;

    case SP_IN_OBJECT:
        break;  // handled below
    }

    switch (s->os) {
    case OS_MEMBERS:
        if (c == '"') {
            s->in_string = true; s->escape = false;
            s->key_len = 0;
            sp_put(s, c);
            s->os = OS_KEY;
        } else if (c == '}') {
            sp_put(s, c);
            s->depth--;              // 1 -> 0
            sp_finalize_object(s);
        } else {
            sp_put(s, c);            // whitespace / commas
        }
        break;

    case OS_KEY:
        sp_put(s, c);
        if (s->escape) {
            s->escape = false;
            if (s->key_len < SP_KEY_CAP - 1) s->key[s->key_len++] = c;
        } else if (c == '\\') {
            s->escape = true;
        } else if (c == '"') {
            s->in_string = false;
            s->key[s->key_len < SP_KEY_CAP ? s->key_len : SP_KEY_CAP - 1] = '\0';
            s->skip_value = (strcmp(s->key, "description") == 0 ||
                             strcmp(s->key, "short_description") == 0);
            s->os = OS_COLON;
        } else {
            if (s->key_len < SP_KEY_CAP - 1) s->key[s->key_len++] = c;
        }
        break;

    case OS_COLON:
        sp_put(s, c);
        if (c == ':') s->os = OS_VALUE_START;
        break;

    case OS_VALUE_START:
        if (isspace(uc)) { sp_put(s, c); break; }
        if (c == '"') {
            s->in_string = true; s->escape = false;
            sp_put(s, c);                 // opening quote
            if (s->skip_value) {          // blank stripped fields -> ""
                sp_put(s, '"');
                s->suppress = true;
            }
            s->os = OS_VALUE;
        } else if (c == '{' || c == '[') {
            s->depth++;
            sp_put(s, c);
            s->os = OS_VALUE;
        } else {
            sp_put(s, c);                 // scalar: number/true/false/null
            s->os = OS_VALUE;
        }
        break;

    case OS_VALUE:
        if (s->in_string) {
            // A closing quote ends the *value* only when this string is the
            // top-level value (depth==1). Strings nested inside a structured
            // value (e.g. a URL inside the occurrences array) must not end it.
            if (s->suppress) {            // dropping a stripped string body
                if (s->escape) s->escape = false;
                else if (c == '\\') s->escape = true;
                else if (c == '"') { s->in_string = false; s->suppress = false;
                                     if (s->depth == 1) s->os = OS_AFTER_VALUE; }
            } else {
                sp_put(s, c);
                if (s->escape) s->escape = false;
                else if (c == '\\') s->escape = true;
                else if (c == '"') { s->in_string = false;
                                     if (s->depth == 1) s->os = OS_AFTER_VALUE; }
            }
            break;
        }
        // structural value (object/array) or scalar
        if (c == '"') { s->in_string = true; s->escape = false; sp_put(s, c); break; }
        if (c == '{' || c == '[') { s->depth++; sp_put(s, c); break; }
        if (c == '}' || c == ']') {
            if (s->depth > 1) {           // closing a nested structure
                s->depth--;
                sp_put(s, c);
                if (s->depth == 1) s->os = OS_AFTER_VALUE;
            } else {                      // scalar value ended at object end
                sp_put(s, c);
                s->depth--;               // 1 -> 0
                sp_finalize_object(s);
            }
            break;
        }
        if (c == ',') {
            if (s->depth == 1) {          // scalar value ended, next member
                sp_put(s, c);
                s->skip_value = false;
                s->os = OS_MEMBERS;
            } else {
                sp_put(s, c);             // comma inside nested structure
            }
            break;
        }
        sp_put(s, c);                     // scalar body chars
        break;

    case OS_AFTER_VALUE:
        if (isspace(uc)) { sp_put(s, c); break; }
        if (c == ',') { sp_put(s, c); s->skip_value = false; s->os = OS_MEMBERS; break; }
        if (c == '}') { sp_put(s, c); s->depth--; sp_finalize_object(s); break; }
        sp_put(s, c);
        break;
    }
}

void sp_feed(sp_state_t* s, const char* buf, int len)
{
    for (int i = 0; i < len; i++) {
        if (s->phase == SP_DONE || s->phase == SP_ERR) return;
        sp_feed_byte(s, buf[i]);
    }
}
