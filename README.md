# Airkiss

由于微信官方的airkiss静态库调试起来很不方便,而且也无法按需要进行拓展,
因此在已公开的Airkiss协议基础上实现了一份源码. 最初版本参考了[勋睿科技][xrf]
开发板里提供的驱动代码.

> 本例程实现的源码在[airkiss\_debugger][akdbg]以及最新版的微信(6.3.13)上测试通过, 但不保证100%兼容微信官方的静态库.

## About Airkiss

[Airkiss][airkiss]是微信提出的一种无线应用层协议,主要用于给无法交互的硬件设备进行网络配置,
如(智能)插座,灯泡,飞机杯等. 其原理是将硬件设备的网卡置于监听模式(monitor mode),
又称为混杂模式(promiscous mode), 从而获取周围的802.11无线数据帧, 俗称抓包.
其原型是TI最早提出的[Smart Config][smartcfg].

具体的实现细节可以参考下列**非官方**的资料和文档:

- [Airkss技术实现方案][airkiss_doc]
- [How does TI CC3000 wifi smart config work?][smartcfg_doc]

## Build

`main.c`在Linux下进行切换wifi模式,切换信道以及抓包,实现了一个简单的airkiss上层应用.
编译和运行过程如下:

    $ make clean
    $ make
    $ sudo ./a.out wlan0

开始运行后可以用微信或者airkiss\_debugger发送wifi密码进行测试, 如我发送了123456789,则有如下输出:

```
air_cfg size:96
Airkiss verson: V1.0
scan all channels
scan all channels
scan all channels
airkiss_recv_discover success
base len:42
Lock channel in 8
airkiss_process_magic_code success
total_len:17, ssid crc:d
airkiss_process_prefix_code success
pswd_len:9, pswd_crc:9c, need seq:3, seq map:7
seq:0, 31,32,33,34
now seq map:1
seq:1, 35,36,37,38
now seq map:3
seq:2, 39,5d,4c,49
now seq map:7
Airkiss completed.
get result:
ssid:(null)
length:0
key:123456789
length:9
ssid_crc:d
random:93
```

上层应用收到wifi帐号密码之后进行连接,然后再向10000端口广播random值即可完成配置.

> 注: Linux下抓包需要用到`libnl-3`, `libnl-genl-3` 以及 `libpcap`, 操作网卡需要root权限.

[xrf]: http://www.xrf.net.cn
[akdbg]: http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDebugger.apk
[airkiss]:http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDoc.pdf
[smartcfg]:http://processors.wiki.ti.com/index.php/CC3000_Smart_Config
[airkiss_doc]:http://wenku.baidu.com/view/a5d51c18561252d380eb6eab.html
[smartcfg_doc]:http://electronics.stackexchange.com/questions/61704/how-does-ti-cc3000-wifi-smart-config-work

