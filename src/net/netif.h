#pragma once

#include <mios/device.h>
#include <mios/task.h>

#include "pbuf.h"
#include "socket.h"

#define NET_MAX_INTERFACES 8

SLIST_HEAD(netif_list, netif);
LIST_HEAD(nexthop_list, nexthop);

struct nexthop;

extern struct netif_list netifs;

typedef struct netif {

  device_t ni_dev;

  struct pbuf_queue ni_rx_queue;

  struct nexthop_list ni_nexthops;

  // FIXME: This needs to be reworks to support multiple addresses
  // and address families per interface
  uint32_t ni_local_addr;  // Our address
  uint8_t ni_local_prefixlen;
  uint8_t ni_ifindex;
  uint16_t ni_mtu;
  pbuf_t *(*ni_output)(struct netif *ni, struct nexthop *nh, pbuf_t *pb);

  void (*ni_periodic)(struct netif *ni);

  void (*ni_buffers_avail)(struct netif *ni);

  struct pbuf *(*ni_input)(struct netif *ni, struct pbuf *pb);

  SLIST_ENTRY(netif) ni_global_link;

  struct socket_list ni_sockets;

} netif_t;



#define NEXTHOP_IDLE       0
#define NEXTHOP_RESOLVE    5
#define NEXTHOP_ACTIVE     10

typedef struct nexthop {
  LIST_ENTRY(nexthop) nh_global_link;
  uint32_t nh_addr;

  LIST_ENTRY(nexthop) nh_netif_link;
  struct netif *nh_netif;

  struct pbuf *nh_pending;

  uint8_t nh_state;
  uint8_t nh_in_use;
  uint8_t nh_hwaddr[6];

} nexthop_t;


void netif_wakeup(netif_t *ni);

void netif_attach(netif_t *ni, const char *name);

extern mutex_t netif_mutex;
