
#include "httpd.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <fcntl.h>
#include "esp_system.h"
#include "esp_log.h"

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath) {
  struct
  {
    const char *extension;
    const char *filetype;
  } filetypes[] = {
      {".html", "text/html"},
      {".js", "application/javascript"},
      {".css", "text/css"},
      {".png", "image/png"},
      {".gif", "image/gif"},
      {".ico", "image/x-icon"},
      {".svg", "image/svg+xml"},
      {NULL, NULL},
  };

  for (int i = 0; filetypes[i].extension != NULL; i++)
    if (CHECK_FILE_EXTENSION(filepath, filetypes[i].extension))
      return httpd_resp_set_type(req, filetypes[i].filetype);
  return httpd_resp_set_type(req, "text/plain");
}

/* Send HTTP response with the contents of the requested file */
esp_err_t get_handler(httpd_req_t *req) {
  char filepath[FILE_PATH_MAX];

  //&rest_context = (rest_server_context_t*)req->user_ctx;
  strlcpy(filepath, BASE_PATH, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/')
    strlcat(filepath, "/index.html", sizeof(filepath));
  else
    strlcat(filepath, req->uri, sizeof(filepath));

  int fd = open(filepath, O_RDONLY);
  if (fd < 0 ) {
    ESP_LOGE(__FILE__, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  ESP_LOGI(__FILE__, "Sending file : %s", filepath);
  set_content_type_from_file(req, filepath);

  char chunk[SCRATCH_BUFSIZE];
  ssize_t read_bytes;

  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);

    if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      ESP_LOGI(__FILE__, "Chunk sending %d bytes", read_bytes);
      if (httpd_resp_send_chunk(req, chunk, read_bytes) == ESP_OK)
        ESP_LOGI(__FILE__, "Chunk sent");
      else {
        close(fd);
        ESP_LOGE(__FILE__, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }

    /* Keep looping till the whole file is sent */
  } while (read_bytes != 0);

  /* Close file after sending complete */
  close(fd);
  /* Respond with an empty chunk to signal HTTP response completion */
  ESP_ERROR_CHECK(httpd_resp_send_chunk(req, NULL, 0));
  ESP_LOGI(__FILE__, "File sending complete");
  return ESP_OK;
}
