#include "utils.h"
#include "wifi_scan.h"
#include <iwlib.h>

static iwrange range;
int wifi_scan(const char *device, wireless_scan_head *result_head)
{
    int sock;
    int success = -1;

    /* Open socket to kernel */
    sock = iw_sockets_open();

    do {
        if(sock < 0) {
            LOG_ERROR("Error to open kernel sockect.");
            break;
        }

        /* Get some metadata to use for scanning */
        if (iw_get_range_info(sock, device, &range) < 0) {
            LOG_ERROR("Error during iw_get_range_info.");
            break;
        }

        /* Perform the scan */
        if (iw_scan(sock, (char *)device, range.we_version_compiled, result_head) < 0) {
            LOG_ERROR("Error during iw_scan.");
            break;
        }

        success = 0;
    } while(0);

    iw_sockets_close(sock);
    return success;
}

void get_essid(wireless_scan *ap, char *essid, unsigned int len)
{
    snprintf(essid, len, "%s", ap->b.essid);
}
void get_bssid(wireless_scan *ap, char *bssid, unsigned int len)
{
    unsigned char mac_addr[7];
    memcpy(mac_addr, ap->ap_addr.sa_data, 7);
    snprintf(bssid, len,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0],
            mac_addr[1],
            mac_addr[2],
            mac_addr[3],
            mac_addr[4],
            mac_addr[5]
            );
}
unsigned int get_freq_mhz(wireless_scan *ap)
{
    unsigned int freq = 0;
    if(ap)
        freq = ap->b.freq/1000000;
    return freq;
}
int get_strength_dbm(wireless_scan *ap)
{
    char buf[128];
    memset(buf, 0, 128);
    iw_print_stats(buf, 128, &ap->stats.qual, &range, 1);
    int qual,qual_all,strength;
    sscanf(buf, "Quality=%d/%d  Signal level=%d dBm  ",
            &qual,&qual_all,&strength);
    return strength;
}


void print_ap_info(wireless_scan *ap)
{
    char essid[MAX_ESSID_SIZE];
    char bssid[MAX_BSSID_SIZE];
    unsigned int freq;
    int strength;
    get_essid(ap, essid, MAX_ESSID_SIZE);
    get_bssid(ap, bssid, MAX_BSSID_SIZE);
    freq = get_freq_mhz(ap);
    strength = get_strength_dbm(ap);

    printf("bssid:[%s], freq:[%d MHz], power:[%d dBm], essid:[%s]\n",
            bssid,
            freq,
            strength,
            essid);
}
