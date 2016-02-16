# Airkiss


## About Airkiss

[Airkiss][airkiss]是微信提出的一种无线应用层协议,主要用于给无法交互的硬件设备进行网络配置,
如(智能)插座,灯泡,飞机杯等. 其原理是将硬件设备的网卡置于监听模式(monitor mode),
又称为混杂模式(promiscuous mode), 从而获取周围的802.11无线数据帧, 俗称抓包. 
加密的无线数据中length字段是可见的,利用这个字段我们就能约定一种传输数据的协议,
从而在硬件设备初次进入环境时为其提供wifi的帐号密码等信息.
其原型是TI最早提出的[Smart Config][smartcfg].

由于微信官方的airkiss静态库调试起来很不方便,而且也无法按需要进行拓展,
因此在已公开的Airkiss协议基础上实现了一份C代码. 最初版本参考了[勋睿科技][xrf]
开发板里提供的驱动程序.

> 注:  
> 本例程实现的源码在[airkiss\_debugger][akdbg]以及最新版的微信(6.3.13)上测试通过, 但不保证100%兼容微信官方的静态库.  

具体的实现细节可以参考下列**非官方**的资料和文档:

- [Airkss技术实现方案][airkiss_doc]
- [How does TI CC3000 wifi smart config work?][smartcfg_doc]

## Build

`main.c`在Linux下进行切换wifi模式,切换信道以及抓包,实现了一个简单的airkiss上层应用.
编译和运行过程如下:

    $ make clean
    $ make
    $ sudo ./a.out wlan0

开始运行后可以用微信或者airkiss\_debugger发送wifi密码进行测试, 如我发送密码0123456789,则有如下输出:

```
NL80211_CMD_TRIGGER_SCAN sent 36 bytes to the kernel.
Waiting for scan to complete...
Got NL80211_CMD_NEW_SCAN_RESULTS.
Scan is done.
NL80211_CMD_GET_SCAN sent 28 bytes to the kernel.
Mac Address:[14:75:90:1b:06:90], Channel:[ 1], SSID_CRC:[89], SSID:[TP-LINK_0690]
Mac Address:[fc:d7:33:87:a3:a6], Channel:[11], SSID_CRC:[ d], SSID:[LINK001]
Mac Address:[08:10:79:16:fa:69], Channel:[ 6], SSID_CRC:[9c], SSID:[orans-sh_1]
Mac Address:[08:10:79:16:f6:c9], Channel:[13], SSID_CRC:[7e], SSID:[orans-sh_2]
Mac Address:[00:23:cd:a0:16:cc], Channel:[11], SSID_CRC:[25], SSID:[WDH]
Mac Address:[1c:fa:68:aa:c2:b8], Channel:[ 3], SSID_CRC:[b5], SSID:[shzy service]
Mac Address:[d2:15:0d:1f:f0:cc], Channel:[11], SSID_CRC:[10], SSID:[Guest_luyisi]
Mac Address:[d8:15:0d:1f:f0:cc], Channel:[11], SSID_CRC:[e9], SSID:[nanmu]
Mac Address:[82:89:17:9f:1f:bf], Channel:[ 1], SSID_CRC:[79], SSID:[nanbo2f]
Mac Address:[08:10:79:16:fa:68], Channel:[ 6], SSID_CRC:[ 0], SSID:[]
air_cfg size:96
Airkiss verson: V1.0
scan all channels
airkiss_recv_discover success
base len:42
Lock channel in 4
airkiss_process_magic_code success
total_len:18, ssid crc:d
airkiss_process_prefix_code success
pswd_len:10, pswd_lencrc:7e, need seq:3, seq map:7
seq:1, d8,d8,d8,d8
CRC check error, invalid sequence, Discared it.
seq:2, 38,39,f7,4c
now seq map:4
seq:3, d8,d8,d8,d8
CRC check error, invalid sequence, Discared it.
seq:4, 30,31,d8,d8
CRC check error, invalid sequence, Discared it.
seq:0, 30,31,32,33
now seq map:5
seq:3, 49,4e,4b,30
now seq map:5
seq:0, 30,31,32,33
now seq map:5
seq:0, 30,31,32,33
now seq map:5
seq:2, 38,39,f7,4c
now seq map:5
seq:3, 49,4e,4b,30
now seq map:5
seq:0, 30,31,32,33
now seq map:5
seq:1, 34,35,36,37
now seq map:7
Airkiss completed.
get result:
ssid_crc:d
key:0123456789
key_len:10
random:247
Sending random to broadcast..
```

一个优化的地方是airkiss上层在抓包之前先扫描附近的无线热点并记录其ssid/crc以及信道,从而使得airkiss
只用在这几个信道切换抓包. 另外为了加快Airkiss进度,ssid部分不从data字段读取而只取其crc(用reserved字段记录),
上层应用将记录的ssid/crc进行对比,则可以获取目标的ssid信息并连接.连接后根据airkiss协议,
向10000端口广播random值通知发送端即可完成配置.

> 注:  
> Linux下抓包需要用到`libnl-3`, `libnl-genl-3` 以及 `libpcap`, 操作网卡需要root权限.  
> 由于不同抓包策略会导致抓到的包格式各不相同,比如有的是带802.11头的数据帧(亦即微信官方要求的格式),  
> 有的是更底层的比如带Radiotap头的数据,更有的是不带头的纯数据,为了彼此兼容,代码里仅仅用到长度来编解码.

## Known issue

- ssid如果为中文则显示为空
- 暂未对wifi密码进行AES加/解密

[xrf]: http://www.xrf.net.cn
[akdbg]: http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDebugger.apk
[airkiss]:http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDoc.pdf
[smartcfg]:http://processors.wiki.ti.com/index.php/CC3000_Smart_Config
[airkiss_doc]:http://wenku.baidu.com/view/0e825981ad02de80d5d8409c
[airkiss_doc2]:https://www.docdroid.net/UIi8rgt/airkiss-protocol.pdf.html
[smartcfg_doc]:http://electronics.stackexchange.com/questions/61704/how-does-ti-cc3000-wifi-smart-config-work

