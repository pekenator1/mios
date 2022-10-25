#pragma once

typedef enum {
  LOG_EMERG,
  LOG_ALERT,
  LOG_CRIT,
  LOG_ERR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG,
} event_level_t;

void evlog(event_level_t level, const char *fmt, ...);
