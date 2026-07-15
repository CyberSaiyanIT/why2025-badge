#include "sync.h"
#include <esp_heap_caps.h>
#include "esp_crt_bundle.h"
#include "schedule_parse.h"

// EMF 2026 runs Thu 16 -> Sun 19 Jul; day-of-month 16 maps to "day 1".
#define EMF_DAY1_DOM 16

// Static HTML welcome shown above the schedule (base64; the web UI atob()s it).
// "<p>Welcome to EMF 2026! Cyber Saiyan is here - enjoy and be respectful to each other</p>"
static const char* SCHEDULE_INFO_B64 =
    "PHA+V2VsY29tZSB0byBFTUYgMjAyNiEgQ3liZXIgU2FpeWFuIGlzIGhlcmUgLSBlbmpveSBhbmQgYmUgcmVzcGVjdGZ1bCB0byBlYWNoIG90aGVyPC9wPg==";

static int64_t last_run = 0;
static int64_t current_run = 0;
static bool errors, forced, connected = false;

// Streaming EMF-feed -> badge-schedule converter state, persisted across the
// HTTP data callbacks. g_scanner.out holds the open .tmp file while downloading.
static sp_state_t g_scanner;
static bool g_out_open = false;

// Close the streaming output file if it is open (safety on any exit path).
static void sync_close_out(void)
{
    if (g_out_open) {
        fclose(g_scanner.out);
        g_out_open = false;
    }
}

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    static int raw_len = 0;          // raw feed bytes received (for logging)

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(__FILE__, "HTTP_EVENT_ERROR");
            raw_len = 0;
            errors = true;
            sync_close_out();
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(__FILE__, "HTTP_EVENT_ON_CONNECTED");

            if(!connected) connected = true;
            raw_len = 0;
            errors = false;

            // Open the temp file and stream the schedule wrapper into it; the
            // scanner writes the row objects between the '[' and ']'.
            sync_close_out();
            FILE* fp = fopen(TMP_SCHEDULE_FILE, "w");
            if (!fp) {
                ESP_LOGE(__FILE__, "Cannot open %s for write", TMP_SCHEDULE_FILE);
                errors = true;
                break;
            }
            if (fprintf(fp, "{\"info\":\"%s\",\"schedule\":[", SCHEDULE_INFO_B64) < 0) {
                ESP_LOGE(__FILE__, "Cannot write schedule header");
                fclose(fp);
                errors = true;
                break;
            }
            sp_init(&g_scanner, fp, EMF_DAY1_DOM);
            g_out_open = true;
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(__FILE__, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(__FILE__, "HTTP_EVENT_ON_HEADER");
            ESP_LOGI(__FILE__, "%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(__FILE__, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            if(errors || !g_out_open)
                return ESP_FAIL;

            // Feed this chunk straight into the streaming parser. Safe across
            // arbitrary chunk boundaries.
            sp_feed(&g_scanner, (const char*)evt->data, evt->data_len);
            raw_len += evt->data_len;

            if (g_scanner.phase == SP_ERR) {   // output write failure
                ESP_LOGE(__FILE__, "Schedule parse/write error");
                errors = true;
                sync_close_out();
                return ESP_FAIL;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(__FILE__, "HTTP_EVENT_ON_FINISH");
            ESP_LOGI(__FILE__, "Received %d raw bytes, parsed %d rows", raw_len, g_scanner.rows);

            if(errors || !g_out_open)
                return ESP_FAIL;

            if(!esp_http_client_is_complete_data_received(evt->client)){
                ESP_LOGE(__FILE__, "Incomplete data received");
                errors = true;
                sync_close_out();
                return ESP_FAIL;
            }

            // Close the JSON wrapper and the file.
            bool ok = (fputs("]}", g_scanner.out) != EOF);
            sync_close_out();

            if (!ok || !sp_ok(&g_scanner)) {
                ESP_LOGE(__FILE__, "Schedule not finalized cleanly (ok=%d, phase=%d)",
                         ok, g_scanner.phase);
                errors = true;
                break;   // leave old SCHEDULE_FILE intact
            }

            struct stat st;
            if (stat(SCHEDULE_FILE, &st) == 0) {
                unlink(SCHEDULE_FILE);
            }

            ESP_LOGI(__FILE__, "Renaming file");
            if (rename(TMP_SCHEDULE_FILE, SCHEDULE_FILE) != 0) {
                ESP_LOGE(__FILE__, "Rename failed");
                errors = true;
                return ESP_FAIL;
            }
            ESP_LOGI(__FILE__, "Schedule saved with %d rows", g_scanner.rows);

            // Mark the schedule stale; the table is rebuilt lazily the next
            // time the event screen is opened (keeps this off the TLS path).
            ui_schedule_reset();
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(__FILE__, "HTTP_EVENT_DISCONNECTED");

            raw_len = 0;
            connected = false;
            sync_close_out();       // safety: never leave the temp file open
            ui_toggle_sync();

            if(errors){
                errors = false;
                return ESP_FAIL;
            }

            last_run = current_run; // update timer
            break;
    }
    return ESP_OK;
}

void schedule_sync_handler(bool force) {
    current_run = esp_timer_get_time();
    forced = force;
    ESP_LOGI(__FILE__, "Previous time: %lld", last_run);
    ESP_LOGI(__FILE__, "Current time: %lld", current_run);
    
    // Check available heap memory before attempting connection
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    ESP_LOGI(__FILE__, "Free heap: %d bytes, Min free heap: %d bytes", free_heap, min_free_heap);
    
    // Need at least 32KB of free memory for SSL connection
    if (free_heap < 32768) {
        ESP_LOGW(__FILE__, "Insufficient memory for SSL connection (%d bytes available)", free_heap);
        return;
    }
    
    if(!connected && (forced || last_run == 0 || (current_run - last_run) > 1000*SYNC_PERIOD_MS))
    {
        ESP_LOGI(__FILE__, "Connecting to %s", badge_obj.sync_url);

        esp_http_client_config_t http_config = {
        .url = badge_obj.sync_url,
        .event_handler = _http_event_handle,
        .crt_bundle_attach = esp_crt_bundle_attach,  // validate TLS via cert bundle
        .timeout_ms = 20000,        // generous for slow / high-latency links
        .buffer_size = 4096,        // response headers (CSP etc.) can be ~1.5 KB
        .buffer_size_tx = 1024,     // Limit TX buffer size
        };
        esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
        if (http_client == NULL) {
            ESP_LOGE(__FILE__, "Failed to initialize HTTP client - insufficient memory");
            return;
        }

        // Plain GET: no Content-Type (it's a leftover that some servers dislike on
        // a bodyless request); send a normal User-Agent instead.
        esp_http_client_set_header(http_client, "User-Agent", "CyberSaiyanBadge/1.0");
        esp_err_t err = esp_http_client_perform(http_client);

        if (err == ESP_OK) {
        ESP_LOGI(__FILE__, "Status = %d, content_length = %" PRId64,
                esp_http_client_get_status_code(http_client),
                esp_http_client_get_content_length(http_client));
        } else {
            ESP_LOGE(__FILE__, "HTTP perform failed: %s", esp_err_to_name(err));
        }
        
        esp_err_t cleanup_err = esp_http_client_cleanup(http_client);
        if(cleanup_err == ESP_OK){
            ESP_LOGI(__FILE__, "HTTP client cleaned up successfully");
        } else {
            ESP_LOGE(__FILE__, "HTTP client cleanup failed: %s", esp_err_to_name(cleanup_err));
        }
    }
}
