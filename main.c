#include "airkiss.h"
#include "osdep.h"
#include "common.h"
#include "wifi_scan.h"
#include <pthread.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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


#define BUF_SIZE	1024
#define MAX_CHANNELS 14

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
char *wifi_if = NULL;
struct wif *wi = NULL;
int channels[MAX_CHANNELS] = {0};
int channel_index = 0;
int channel_nums = 0;
pthread_mutex_t lock;

void switch_channel_callback(void)
{
    pthread_mutex_lock(&lock);
    //printf("Current channel is: %d\n", channel);
    channel_index++;
    if(channel_index > channel_nums - 1)
    {
        channel_index = 0;
        printf("scan all channels\n");
    }
	int ret = wi->wi_set_channel(wi, channels[channel_index]);
	if (ret) {
		printf("cannot set channel to %d\n", channels[channel_index]);
	}

    airkiss_change_channel(akcontex);
    pthread_mutex_unlock(&lock);
}

int process_airkiss(const unsigned char *packet, int size)
{
    pthread_mutex_lock(&lock);
    int ret;
    ret = airkiss_recv(akcontex, (void *)packet, size);
    if(ret==AIRKISS_STATUS_CONTINUE)
    {
        ;//
    }
    else if(ret == AIRKISS_STATUS_CHANNEL_LOCKED)
    {
        startTimer(&my_timer, 0);
        printf("Lock channel in %d\n", channels[channel_index]);
    }
    else if(ret == AIRKISS_STATUS_COMPLETE)
    {
        printf("Airkiss completed.\n");
        airkiss_get_result(akcontex, &ak_result);
        printf("get result: ssid_crc:%x\nkey:%s\nkey_len:%d\nrandom:%d\n", 
            ak_result.reserved, ak_result.pwd, ak_result.pwd_length, ak_result.random);

        //TODO: scan and connect to wifi
        udp_broadcast(ak_result.random, 10000);
    }
    pthread_mutex_unlock(&lock);

    //print  header
    //printf("[len: %d, airkiss ret: %d]\n", size, ret);
    //int i;
    //unsigned char ch;
    //for(i=0; i<24; i++)
    //{
    //    ch = (unsigned char)*(packet + i);
    //    printf("0x%02x ", ch);
    //}
    //printf("\n");
    return ret;
}

void add_channel(int chan) {
    int i;
    for(i=0; i<channel_nums; i++) {
        if(channels[i]==chan)
            break;
    }
    if(i==channel_nums) {
        channel_nums += 1;
        channels[i] = chan;
    }
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
    int _channel = getChannelFromFrequency(nla_get_u32(bss[NL80211_BSS_FREQUENCY]));
    add_channel(_channel);
    printf("Channel:[%2d], ", _channel);
    printf("SSID_CRC:[%2x], SSID:[%s]", calcrc_bytes((unsigned char *)ssid, strlen(ssid)), ssid);
    printf("\n");

    return NL_SKIP;
}

void init_channels()
{
    int i;
    //add channel 1-13
    for(i=1; i<MAX_CHANNELS; i++)
    {
        add_channel(i);
    }
}

int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        printf("Usage: %s <device-name>\n", argv[0]);
        return -1;
    }
    wifi_if = argv[1];
	wi = wi_open(wifi_if);
	if (!wi) {
		printf("cannot init interface %s\n", wifi_if);
		return 1;
	}

    init_channels();
    //wifi_scan(wifi_if, &scan_callback);

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
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("mutex init failed\n");
        return 1;
    }

    /* Setup channel switch timer */
    startTimer(&my_timer, 200);   
    signal(SIGALRM,(__sighandler_t)&switch_channel_callback);
    
   
    int read_size;
	unsigned char buf[BUF_SIZE] = {0};
	while (1) {
		read_size = wi->wi_read(wi, buf, BUF_SIZE, NULL);
		if (read_size < 0) {

			printf("recv failed, ret %d\n", read_size);
			break;
		}
        if(AIRKISS_STATUS_COMPLETE==process_airkiss(buf, read_size))
            break;
	}
    pthread_mutex_destroy(&lock);
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
    return 0;
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
