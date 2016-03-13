#include "airkiss.h"
#include "wifi_scan.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <linux/nl80211.h>
#include <linux/genetlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <pcap.h>
#include <net/if.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>

/* nl80211 */
struct nl80211_state {
    struct nl_sock *nl_sock;
    struct nl_cache *nl_cache;
    struct genl_family *nl80211;
} state;

/**
 * Return the channel from the frequency (in Mhz)
 */
int getChannelFromFrequency(int frequency)
{
    if (frequency >= 2412 && frequency <= 2472)
        return (frequency - 2407) / 5;
    else if (frequency == 2484)
        return 14;

    else if (frequency >= 4920 && frequency <= 6100)
        return (frequency - 5000) / 5;
    else
        return -1;
}

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
    /* see 802.11 17.3.8.3.2 and Annex J
     * there are overlapping channel numbers in 5GHz and 2GHz bands */
    if (chan <= 0)
        return 0; /* not supported */
    switch (band) {
    case NL80211_BAND_2GHZ:
        if (chan == 14)
            return 2484;
        else if (chan < 14)
            return 2407 + chan * 5;
        break;
    case NL80211_BAND_5GHZ:
        if (chan >= 182 && chan <= 196)
            return 4000 + chan * 5;
        else
            return 5000 + chan * 5;
        break;
    case NL80211_BAND_60GHZ:
        if (chan < 5)
            return 56160 + chan * 2160;
        break;
    default:
        ;
    }
    return 0; /* not supported */
}

static int linux_nl80211_init(struct nl80211_state *state)
{
    int err;

    state->nl_sock = nl_socket_alloc();

    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
        fprintf(stderr, "Failed to allocate generic netlink cache.\n");
        err = -ENOMEM;
        goto out_handle_destroy;
    }

    state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
    if (!state->nl80211) {
        fprintf(stderr, "nl80211 not found.\n");
        err = -ENOENT;
        goto out_cache_free;
    }

    return 0;

 out_cache_free:
    nl_cache_free(state->nl_cache);
 out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
    genl_family_put(state->nl80211);
    nl_cache_free(state->nl_cache);
    nl_socket_free(state->nl_sock);
}
static int linux_set_channel_nl80211(const char *if_name, int channel)
{
    unsigned int devid;
    struct nl_msg *msg;
    unsigned int freq;
    unsigned int htval = NL80211_CHAN_NO_HT;
    msg=nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "failed to allocate netlink message\n");
        return 2;
    }

    devid=if_nametoindex(if_name);

    enum nl80211_band band;
    band = channel <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
    freq=ieee80211_channel_to_frequency(channel, band);

    genlmsg_put(msg, 0, 0, genl_family_get_id(state.nl80211), 0,
            0, NL80211_CMD_SET_WIPHY, 0);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);

    nl_send_auto_complete(state.nl_sock,msg);
    nlmsg_free(msg);

    return 0;

 nla_put_failure:
    return -ENOBUFS;
}



pcap_t *handle;                         /* Pcap session handle */

static airkiss_context_t *akcontex = NULL;
const airkiss_config_t akconf = { 
(airkiss_memset_fn)&memset, 
(airkiss_memcpy_fn)&memcpy, 
(airkiss_memcmp_fn)&memcmp, 
(airkiss_printf_fn)&printf };

airkiss_result_t ak_result;

struct itimerval my_timer;
int startTimer(struct itimerval *timer, int ms);
int udp_broadcast(unsigned char random, int port);
static int channel = 1;
const char *wifi_if = NULL;
void switch_channel_callback(void)
{
    //printf("Current channel is: %d\n", channel);
    linux_set_channel_nl80211(wifi_if, channel++);
    if(channel > 14)
    {
        channel = 1;
        printf("scan all channels\n");
    }
}

void recv_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    int ret;
    ret = airkiss_recv(akcontex, (void *)packet, header->len);
    if(ret==AIRKISS_STATUS_CONTINUE)
    {
        return;
    }
    else if(ret == AIRKISS_STATUS_CHANNEL_LOCKED)
    {
        startTimer(&my_timer, 0);
        printf("Lock channel in %d\n", channel);
    }
    else if(ret == AIRKISS_STATUS_COMPLETE)
    {
        printf("Airkiss completed.\n");
        airkiss_get_result(akcontex, &ak_result);
        printf("get result:\nssid_crc:%x\nkey:%s\nkey_len:%d\nrandom:%d\n", 
            ak_result.reserved, ak_result.pwd, ak_result.pwd_length, ak_result.random);

        //TODO: scan and connect to wifi
        udp_broadcast(ak_result.random, 10000);
        pcap_close(handle);
    }
    //printf("[len: %d, airkiss ret: %d]\n", header->len, ret);
    ////print  header
    //int i;
    //unsigned char ch;
    //for(i=0; i<24; i++)
    //{
    //    ch = (unsigned char)*(packet + i);
    //    printf("0x%02x ", ch);
    //}
    //printf("\n");
}

// Called by the kernel with a dump of the successful scan's data. Called for each SSID.
static int scan_callback(struct nl_msg *msg, void *arg)
{
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    char mac_addr[20];
    char ssid[30] = {0};
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_TSF] = { .type = NLA_U64 },
        [NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
        [NL80211_BSS_BSSID] = { },
        [NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
        [NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
        [NL80211_BSS_INFORMATION_ELEMENTS] = { },
        [NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
        [NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
        [NL80211_BSS_STATUS] = { .type = NLA_U32 },
        [NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
        [NL80211_BSS_BEACON_IES] = { },
    };

    // Parse and error check.
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[NL80211_ATTR_BSS]) {
        printf("bss info missing!\n");
        return NL_SKIP;
    }
    if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
        printf("failed to parse nested attributes!\n");
        return NL_SKIP;
    }
    if (!bss[NL80211_BSS_BSSID]) return NL_SKIP;
    if (!bss[NL80211_BSS_INFORMATION_ELEMENTS]) return NL_SKIP;

    // Start printing.
    mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_BSSID]));
    get_ssid(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]), ssid);
    printf("Mac Address:[%s], ", mac_addr);
    //printf("Frequency:[%d MHz], ", nla_get_u32(bss[NL80211_BSS_FREQUENCY]));
    printf("Channel:[%2d], ", getChannelFromFrequency(nla_get_u32(bss[NL80211_BSS_FREQUENCY])));
    printf("SSID_CRC:[%2x], SSID:[%s]", calcrc_bytes((unsigned char *)ssid, strlen(ssid)), ssid);
    printf("\n");

    return NL_SKIP;
}

int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        printf("Usage: %s <device-name>\n", argv[0]);
        return -1;
    }
    wifi_if = argv[1];

    char *dev;
    char errbuf[PCAP_ERRBUF_SIZE];

    dev = pcap_lookupdev(errbuf);
    if(NULL==dev)
    {
        printf("Error: %s\n", errbuf);
        return -1;
    }
    wifi_scan(dev, &scan_callback);

    handle = pcap_open_live(dev, BUFSIZ, 1, 5, errbuf); //5ms recv timeout
    if(NULL==handle)
    {
        printf("Error: %s\n", errbuf);
        return -1;
    }
    /* airkiss setup */
    int result;
    akcontex = (airkiss_context_t *)malloc(sizeof(airkiss_context_t));
    result = airkiss_init(akcontex, &akconf);
    if(result != 0)
    {
        printf("Airkiss init failed!!\n");
        exit(1);
    }
    printf("Airkiss version: %s\n", airkiss_version());

    /* 80211 */
    linux_nl80211_init(&state);

    /* Setup channel switch timer */
    startTimer(&my_timer, 100);   
    signal(SIGALRM,(__sighandler_t)&switch_channel_callback);
    
    pcap_loop(handle, -1, recv_callback, NULL);
    nl80211_cleanup(&state);

    return 0;
}

int startTimer(struct itimerval *timer, int ms)
{
    time_t secs, usecs;
    secs = ms/1000;
    usecs = ms%1000 * 1000;

    timer->it_interval.tv_sec = secs;
    timer->it_interval.tv_usec = usecs;
    timer->it_value.tv_sec = secs;
    timer->it_value.tv_usec = usecs;

    setitimer(ITIMER_REAL, timer, NULL);
}

int udp_broadcast(unsigned char random, int port)
{
    int fd;
    int enabled = 1;
    int err;
    struct sockaddr_in addr;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(port);
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        printf("get socket err:%d", errno);
        return 1;
    } 
    
    err = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &enabled, sizeof(enabled));
    if(err == -1)
    {
        close(fd);
        return 1;
    }
    
    printf("Sending random to broadcast..\n");
    int i;
    unsigned int usecs = 1000*20;
    for(i=0;i<50;i++)
    {
        sendto(fd, (unsigned char *)&random, 1, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
        usleep(usecs);
    }

    close(fd);
}
