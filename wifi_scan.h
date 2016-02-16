#ifndef WIFI_SCAN
#define WIFI_SCAN

#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <linux/nl80211.h>
typedef int (*wifi_scan_callback)(struct nl_msg *msg, void *arg);
int wifi_scan(const char *device, wifi_scan_callback callback);
void print_ssid(unsigned char *ie, int ielen);
void mac_addr_n2a(char *mac_addr, unsigned char *arg);
#endif
