#ifndef AIRKISS_H_
#define AIRKISS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TODO
 * 定义AIRKISS_ENABLE_CRYPT为1以启用AirKiss加密功能
 */
#ifndef AIRKISS_ENABLE_CRYPT
#define AIRKISS_ENABLE_CRYPT 0
#endif

typedef void* (*airkiss_memset_fn) (void* ptr, int value, unsigned int num);
typedef void* (*airkiss_memcpy_fn) (void* dst, const void* src, unsigned int num);
typedef int (*airkiss_memcmp_fn) (const void* ptr1, const void* ptr2, unsigned int num);
typedef int (*airkiss_printf_fn) (const char* format, ...);


typedef struct
{

    airkiss_memset_fn memset;
    airkiss_memcpy_fn memcpy;
    airkiss_memcmp_fn memcmp;
    airkiss_printf_fn printf;

} airkiss_config_t;

typedef struct
{
    int dummyap[26];
    int dummy[32];
} airkiss_context_t;

/*
 * AirKiss解码成功后的结果
 */
typedef struct
{
    char* pwd;                        /* wifi密码，以'\0'结尾 */
    char* ssid;                        /* wifi ssid，以'\0'结尾 */
    unsigned char pwd_length;        /* wifi密码长度 */
    unsigned char ssid_length;        /* wifi ssid长度 */
    unsigned char random;            /* 随机值，根据AirKiss协议，当wifi连接成功后，需要通过udp向10000端口广播这个随机值，这样AirKiss发送端（微信客户端或者AirKissDebugger）就能知道AirKiss已配置成功 */
    unsigned char reserved;            /* 保留值,这里用来保存ssid的crc校验值 */
} airkiss_result_t;


/*
 * airkiss_recv()正常情况下的返回值
 */
typedef enum
{
    /* 解码正常，无需特殊处理，继续调用airkiss_recv()直到解码成功 */
    AIRKISS_STATUS_CONTINUE = 0,

    /* wifi信道已经锁定，上层应该立即停止切换信道 */
    AIRKISS_STATUS_CHANNEL_LOCKED = 1,

    /* 解码成功，可以调用airkiss_get_result()取得结果 */
    AIRKISS_STATUS_COMPLETE = 2

} airkiss_status_t;



#if AIRKISS_ENABLE_CRYPT

/*
 * 设置解密key，最长可以为128bit，若传入的key不足128bit，则默认用0填充
 *
 * 返回值
 *         < 0：出错，通常是参数错误
 *           0：成功
 */
int airkiss_set_key(airkiss_context_t* context, const unsigned char* key, unsigned int length);

#endif

/*
 * Get airkiss version
 */
const char* airkiss_version(void);


unsigned char calcrc_bytes(unsigned char *p,unsigned int num_of_bytes);

/*
 * 初始化AirKiss库
 *
 * 返回值
 *         < 0：出错，通常是参数错误
 *           0：成功
 */
int airkiss_init(airkiss_context_t* context, const airkiss_config_t* config);


/*
 * 开启WiFi Promiscuous Mode后，将收到的包传给airkiss_recv以进行解码
 *
 * 参数说明
 *         frame：802.11 frame mac header(must contain at least first 24 bytes)
 *         length：total frame length
 *
 * 返回值
 *          < 0：出错，通常是参数错误
 *         >= 0：成功，请参考airkiss_status_t
 */
int airkiss_recv(airkiss_context_t* context, const void* frame, unsigned short length);


/*
 * 当airkiss_recv()返回AIRKISS_STATUS_COMPLETE后，调用此函数来获取AirKiss解码结果
 *
 * 返回值
 *         < 0：出错，解码状态还不是AIRKISS_STATUS_COMPLETE
 *           0：成功
 */
int airkiss_get_result(airkiss_context_t* context, airkiss_result_t* result);

/*
 * 上层切换信道以后，可以调用一下本接口清缓存，降低锁定错信道的概率，注意调用的逻辑是在airkiss_init之后
 *
 * 返回值
 *         < 0：出错，通常是参数错误
 *           0：成功
 */
int airkiss_change_channel(airkiss_context_t* context);

#ifdef __cplusplus
}
#endif

#endif 

