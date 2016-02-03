# Airkiss

由于微信官方的airkiss静态库调试起来很不方便,而且也无法按需要进行拓展,
因此在已公开的Airkiss协议基础上实现了一份源码. 最初版本参考了[瑞讯科技][ruix]
开发板里提供的驱动代码.

> 本项目实现的源码在[airkiss\_debugger][akdbg]以及最新版的微信(6.3.13)上测试通过, 但不保证100%兼容微信官方的静态库.

## About Airkiss

[Airkiss][airkiss]是微信提出的一种无线应用层协议,主要用于给无法交互的硬件设备进行网络配置,
如(智能)插座,灯泡,飞机杯等. 其原理是将硬件设备的网卡置于监听模式(monitor mode),
又称为混杂模式(promiscous mode), 从而获取周围的802.11无线数据帧, 俗称抓包.
其原型是TI最早提出的[Smart Config][smartcfg].

具体的实现细节可以参考下列**非官方**的资料和文档:

- [Airkss技术实现方案][airkiss_doc]
- [How does TI CC3000 wifi smart config work?][smartcfg_doc]

[ruix]: http://pannzh.github.io
[akdbg]: http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDebugger.apk
[airkiss]:http://iot.weixin.qq.com/wiki/doc/wifi/AirKissDoc.pdf
[smartcfg]:http://processors.wiki.ti.com/index.php/CC3000_Smart_Config
[airkiss_doc]:http://wenku.baidu.com/view/a5d51c18561252d380eb6eab.html
[smartcfg_doc]:http://electronics.stackexchange.com/questions/61704/how-does-ti-cc3000-wifi-smart-config-work

