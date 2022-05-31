# Intelligent-Building-Fire-Protection-System
该项目是一个基于华为LiteOS的智慧楼宇消防系统，主要技术包含传感器，LiteOS, NB-IoT, 2G, 华为云等。基于小熊派智慧烟感案例进行开发。
参考了华为liteOS开源社区的帖子,社区链接：https://bbs.huaweicloud.com/forum/thread-66201-1-1.html
物联网三层架构每层的设计如下：
1. 感知层：主要由烟雾传感器，火焰传感器感知环境进行数据采集，并在LCD显示屏上显示。STM32L431控制执行器（继电器），通信模组（NB-IoT模组BC35-G, GSM模组SIM800C），报警端(语音模块JQ8900，LED)执行对应的功能。
2. 网络层：主要负责烟雾数值、报警状态值数据的上传（BC35-G）和拨打电话（SIM800C）。
3. 应用层：华为云负责平台数据包解析，实时显示数据并推送报警信息（短信报警和华为云平台报警）。
