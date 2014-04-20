contiki_cc2530_iar
==================

使用IAR建立contiki工程，平台为CC2530。

    【工具链】——IAR EW8051 8.10
    【contiki版本】——contiki 2.7发行版
    【修改说明】
    【1】button-sensor.h重名文件，取消core/dev文件夹中的button-sensor.h，保留platform/cc2530dk中的头文件。
    【2】去除core/dev sensor.c和sensor.h中大多数const，防止编译错误。
    【3】配合第2点，去除button-sensor.c/h adc-sensor.c/h 中大多数const，防止编译错误。
    
    【使用说明】
    【1】IAR工程位于 project/iar/simple 文件夹中。IAR工程运行udp例程没有重启现象。
    【2】每个工程选择项对应一个例程，包括hello-world，blink-hello，timer-test，sensor-demo，udp-server和udp-client。注意，使用udp例程时，请修改udp-client中目标IP地址的设定——请参考【contiki学习笔记——IEEE802.15.4地址变为IPv6地址】
    【3】非常抱歉，边界路由的例程始终存在问题，如果您解决了该问题请邮件xukai19871105@126.com，我也会持续努力解决该问题。
