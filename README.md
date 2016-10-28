# Airkiss


## About Airkiss

[Airkiss][airkiss]是微信提出的一种无线应用层协议,主要用于给无法交互的硬件设备进行网络配置,
如(智能)插座,灯泡,飞机杯等. 其原理是将硬件设备的网卡置于监听模式(monitor mode),
又称为混杂模式(promiscuous mode), 从而获取周围的802.11无线数据帧, 俗称抓包. 
加密的无线数据中length字段是可见的,利用这个字段我们就能约定一种传输数据的协议,
从而在硬件设备初次进入环境时为其提供wifi的帐号密码等信息.
其原型是TI最早提出的[Smart Config][smartcfg].

由于微信官方的airkiss静态库调试起来很不方便,而且也无法按需要进行拓展,
因此在已公开的Airkiss协议基础上实现了一份C代码. 

> 注:  
> 本例程实现的源码在[airkiss\_debugger][akdbg]以及微信(6.3.13)上测试通过, 但不保证100%兼容微信官方的静态库.  

具体的实现细节可以参考下列**非官方**的资料和文档:

- [Airkss技术实现方案][airkiss_doc]
- [How does TI CC3000 wifi smart config work?][smartcfg_doc]

## Build

`main.c`在Linux下进行切换wifi模式,切换信道以及抓包,实现了一个简单的airkiss上层应用.
编译过程如下:

> Linux下扫描热点需要用到`libnl-3`和`libnl-genl-3`, 如果不扫描热点(wifi_scan)则可以不用依赖这两个库.
> 操作网卡如切换信道,切换模式等需要root权限.  



```
$ sudo apt-get install libnl-3-dev libnl-genl-3-dev
$ make clean
$ make
```

## Run

运行时需要关闭占用网卡的进程,如NetworkManager等:


```
$ sudo service NetworkManager stop
或者
$ sudo systemctl stop NetworkManager.service
```

运行程序,抓包和切换信道需要root权限:

```
$ sudo ./airkiss wlan1
```

其中wlan1是所选择的无线网卡, 开始运行后可以用微信或者airkiss_debugger发送wifi密码进行测试, 
如发送密码123456789,则有如下输出:

```
NL80211_CMD_TRIGGER_SCAN sent 36 bytes to the kernel.
Waiting for scan to complete...
Got NL80211_CMD_NEW_SCAN_RESULTS.
Scan is done.
NL80211_CMD_GET_SCAN sent 28 bytes to the kernel.
Mac Address:[80:89:17:48:ae:96], Channel:[ 1], SSID_CRC:[78], SSID:[TP-LINK_AE96]
Mac Address:[f4:6a:92:26:96:fc], Channel:[ 2], SSID_CRC:[cc], SSID:[zhou'zhou]
Mac Address:[a4:56:02:73:bb:03], Channel:[ 1], SSID_CRC:[40], SSID:[360WiFi-73BB03]
Mac Address:[98:bc:57:46:97:73], Channel:[ 1], SSID_CRC:[ 2], SSID:[ChinaNGB-YDw1ap]
Mac Address:[f8:d1:11:f1:3f:d6], Channel:[ 1], SSID_CRC:[e3], SSID:[manson]
Mac Address:[80:89:17:15:9d:a8], Channel:[ 1], SSID_CRC:[69], SSID:[chinanet]
Mac Address:[30:fc:68:05:35:3a], Channel:[ 1], SSID_CRC:[50], SSID:[maomao]
Mac Address:[24:69:68:01:78:80], Channel:[ 1], SSID_CRC:[3f], SSID:[TP-LINK_7880]
Mac Address:[0c:82:68:95:ff:24], Channel:[ 1], SSID_CRC:[2a], SSID:[TP-LINK_95FF24]
Mac Address:[88:25:93:14:65:ca], Channel:[ 6], SSID_CRC:[b2], SSID:[365/64/602]
Mac Address:[fc:d7:33:32:41:5a], Channel:[ 1], SSID_CRC:[89], SSID:[zhao]
Mac Address:[50:bd:5f:84:6d:d9], Channel:[ 1], SSID_CRC:[e3], SSID:[365/63/602]
Mac Address:[bc:d1:77:3a:06:64], Channel:[13], SSID_CRC:[e7], SSID:[MERCURY_802]
air_cfg size:96
Airkiss version: V1.2
exec cmd: iw wlan1 set channel 1
exec cmd: iw wlan1 set channel 2
exec cmd: iw wlan1 set channel 6
exec cmd: iw wlan1 set channel 13
scan all channels
exec cmd: iw wlan1 set channel 1
exec cmd: iw wlan1 set channel 2
airkiss_recv_discover success
base len:76
Lock channel in 2
airkiss_process_magic_code success
total_len:22, ssid crc:78
airkiss_process_prefix_code success
pswd_len:9, pswd_lencrc:9c, need seq:3, seq map:7
seq:0, 31,32,33,34
now seq map:1
seq:1, 35,36,37,38
now seq map:3
seq:3, 2d,4c,49,4e
now seq map:3
seq:1, 35,36,37,38
now seq map:3
seq:0, 31,32,34,35
CRC check error, invalid sequence, Discared it.
seq:3, 2d,4c,49,4e
now seq map:3
seq:0, 31,32,33,34
now seq map:3
seq:2, 39,b7,54,50
now seq map:7
Airkiss completed.
get result:
reserved:78
key:123456789
key_len:9
random:183
Sending random to broadcast..
```

一个优化的地方是airkiss上层在抓包之前先扫描附近的无线热点并记录其ssid/crc以及信道,从而使得airkiss
只用在这几个信道切换抓包. 另外为了加快Airkiss进度,ssid部分不从data字段读取而只取其crc(用reserved字段记录),
上层应用将记录的ssid/crc进行对比,则可以获取目标的ssid信息并连接.连接后根据airkiss协议,
向10000端口广播random值通知发送端即可完成配置.

> 注:  
> 由于不同抓包策略会导致抓到的包格式各不相同,比如有的是带802.11头的数据帧(亦即微信官方要求的格式),  
> 有的是更底层的比如带Radiotap头的数据,更有的是不带头的纯数据(比如同一局域网内),为了彼此兼容,
> 理论上可以仅用长度来编解码. 但实践中发现,如果不对数据帧进行筛选,周围无线信号过多时会造成很大干扰,
> 从而导致无法在指定时间内完成, 因此代码里规定数据为80211数据帧,并对其24位header进行一定程度的过滤.

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

