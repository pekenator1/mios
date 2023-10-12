#pragma once

#include <stdint.h>
#include <mios/mios.h>
#include <mios/bumpalloc.h>
#include <sys/queue.h>

struct stream;


typedef enum {
  HST_HTTP_REQ,
  HST_WEBSOCKET_PACKET,
} http_server_task_type_t;

typedef struct http_server_task {

  STAILQ_ENTRY(http_server_task) hst_global_link;

  TAILQ_ENTRY(http_server_task) hst_connection_link;

  struct http_connection *hst_hc;

  uint8_t hst_type;
  uint8_t hst_opcode;

} http_server_task_t;


typedef struct http_server_wsp {
  http_server_task_t hsw_hst;

  size_t hsw_capacity;
  size_t hsw_used;
  uint8_t hsw_data[0];

} http_server_wsp_t;


typedef struct http_request {

  http_server_task_t hr_hst;

  // Headers
  char *hr_url;
  char *hr_host;
  char *hr_content_type;
  char *hr_upgrade;
  char *hr_connection;
  char *hr_wskey;

  void *hr_body;
  size_t hr_body_size;

  uint16_t hr_header_err;
  uint16_t hr_piggyback_503;

  uint16_t hr_header_match;
  uint8_t hr_header_match_len;
  uint8_t hr_should_keep_alive;

  balloc_t hr_bumpalloc;

} http_request_t;

struct stream *http_response_begin(struct http_request *hr,
                                   int status_code,
                                   const char *content_type);

int http_response_end(struct http_request *hr);

typedef struct http_route {

  const char *hr_path;

  int (*hr_callback)(struct http_request *hr, int argc, const char **argv);

} http_route_t;

#define HTTP_ROUTE_DEF(path, cb) \
  static const http_route_t MIOS_JOIN(rpc, __LINE__) __attribute__ ((used, section("httproute"))) = { path, cb};
