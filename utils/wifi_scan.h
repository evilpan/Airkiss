#ifndef WIFI_SCAN
#define WIFI_SCAN

#include <iwlib.h>
#define MAX_ESSID_SIZE 255
#define MAX_BSSID_SIZE 18
int wifi_scan(const char *device, wireless_scan_head *result_head);
unsigned int get_freq_mhz(wireless_scan *ap);
void get_bssid(wireless_scan *ap, char *bssid, unsigned int len);
void get_essid(wireless_scan *ap, char *essid, unsigned int len);
int get_strength_dbm(wireless_scan *ap);

void print_ap_info(wireless_scan *ap);
#endif
