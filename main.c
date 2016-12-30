#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "capture/common.h"
#include "capture/osdep.h"
#include "utils/utils.h"
#include "utils/wifi_scan.h"

#include "airkiss.h"

#define MAX_CHANNELS 14
#define DEBUG 0

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

int g_channels[MAX_CHANNELS] = {0};
int g_channel_index = 0;
int g_channel_nums = 0;
pthread_mutex_t lock;

void switch_channel_callback(void)
{
    pthread_mutex_lock(&lock);
    g_channel_index++;
    if(g_channel_index > g_channel_nums - 1)
    {
        g_channel_index = 0;
        LOG_TRACE("scan all channels");
    }
	int ret = wi->wi_set_channel(wi, g_channels[g_channel_index]);
	if (ret) {
		LOG_TRACE("cannot set channel to %d", g_channels[g_channel_index]);
	}

    airkiss_change_channel(akcontex);
    pthread_mutex_unlock(&lock);
}

int process_airkiss(const unsigned char *packet, int size)
{
    pthread_mutex_lock(&lock);
    int ret;
    ret = airkiss_recv(akcontex, (void *)packet, size);
    if(ret == AIRKISS_STATUS_CONTINUE)
    {
        ;//
    }
    else if(ret == AIRKISS_STATUS_CHANNEL_LOCKED)
    {
        startTimer(&my_timer, 0);
        LOG_TRACE("Lock channel in %d", g_channels[g_channel_index]);
    }
    else if(ret == AIRKISS_STATUS_COMPLETE)
    {
        LOG_TRACE("Airkiss completed.");
        airkiss_get_result(akcontex, &ak_result);
        LOG_TRACE("Result:\nssid_crc:[%x]\nkey_len:[%d]\nkey:[%s]\nrandom:[0x%02x]", 
            ak_result.reserved,
            ak_result.pwd_length,
            ak_result.pwd,
            ak_result.random);

        //TODO: scan and connect to wifi
        
        udp_broadcast(ak_result.random, 10000);
    }
    pthread_mutex_unlock(&lock);

    if(DEBUG) {
        /* print  header */
        printf("len:%4d, airkiss ret:%d [ ", size, ret);
        int i;
        unsigned char ch;
        for(i=0; i<24; i++)
        {
            ch = (unsigned char)*(packet + i);
            printf("%02x ", ch);
        }
        printf("]\n");
    }
    return ret;
}

void add_channel(int chan) {
    int i;
    for(i=0; i<g_channel_nums; i++) {
        if(g_channels[i]==chan)
            break;
    }
    if(i==g_channel_nums) {
        g_channel_nums += 1;
        g_channels[i] = chan;
    }
}

void init_channels()
{
    int i;
    for(i=1; i<MAX_CHANNELS; i++)
    {
        add_channel(i);
    }
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
        LOG_TRACE("Error to create socket, reason: %s", strerror(errno));
        return 1;
    } 
    
    err = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &enabled, sizeof(enabled));
    if(err == -1)
    {
        close(fd);
        return 1;
    }
    
    LOG_TRACE("Sending random to broadcast..");
    int i;
    useconds_t usecs = 1000*20;
    for(i=0;i<50;i++)
    {
        sendto(fd, (unsigned char *)&random, 1, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
        usleep(usecs);
    }

    close(fd);
    return 0;
}


int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        LOG_ERROR("Usage: %s <device-name>", argv[0]);
        return 1;
    }
    wifi_if = argv[1];

    wireless_scan_head head;
    wireless_scan *presult = NULL;
    LOG_TRACE("Scanning accesss point...");
    if(wifi_scan(wifi_if, &head) == 0)
    {
        LOG_TRACE("Scan success.");
        presult = head.result;
        while(presult != NULL) {
            char essid[MAX_ESSID_SIZE];
            char bssid[MAX_BSSID_SIZE];
            unsigned int freq;
            int channel,power;
            unsigned char essid_crc;

            get_essid(presult, essid, MAX_ESSID_SIZE);
            get_bssid(presult, bssid, MAX_BSSID_SIZE);
            freq = get_freq_mhz(presult);
            power = get_strength_dbm(presult);

            channel = getChannelFromFrequency(freq);
            essid_crc = calcrc_bytes((unsigned char*)essid, strlen(essid));

            LOG_TRACE("bssid:[%s], channel:[%2d], pow:[%d dBm], essid_crc:[%02x], essid:[%s]",
                    bssid, channel, power, essid_crc, essid);
            add_channel(channel);
            presult = presult->next;
        }
    }
    else
    {
        LOG_ERROR("ERROR to scan AP, init with all %d channels", MAX_CHANNELS);
        init_channels();
    }

    /* Open the interface and set mode monitor */
	wi = wi_open(wifi_if);
	if (!wi) {
		LOG_ERROR("cannot init interface %s", wifi_if);
		return 1;
	}


    /* airkiss setup */
    int result;
    akcontex = (airkiss_context_t *)malloc(sizeof(airkiss_context_t));
    result = airkiss_init(akcontex, &akconf);
    if(result != 0)
    {
        LOG_ERROR("Airkiss init failed!!");
        return 1;
    }
    LOG_TRACE("Airkiss version: %s", airkiss_version());
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
        LOG_ERROR("mutex init failed");
        return 1;
    }

    /* Setup channel switch timer */
    startTimer(&my_timer, 400);   
    signal(SIGALRM,(__sighandler_t)&switch_channel_callback);
    
   
    int read_size;
	unsigned char buf[RECV_BUFSIZE] = {0};
	for(;;)
    {
		read_size = wi->wi_read(wi, buf, RECV_BUFSIZE, NULL);
		if (read_size < 0) {
			LOG_ERROR("recv failed, ret %d", read_size);
			break;
		}
        if(AIRKISS_STATUS_COMPLETE==process_airkiss(buf, read_size))
            break;
	}

    free(akcontex);
    pthread_mutex_destroy(&lock);
    return 0;
}
