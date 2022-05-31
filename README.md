# Intelligent-Building-Fire-Protection-System
该项目是一个基于华为LiteOS的智慧楼宇消防系统，主要技术包含传感器，LiteOS, NB-IoT, 2G, 华为云等。基于小熊派智慧烟感案例进行开发。
一、文件说明：
  Smoke.zip是完整的项目代码。（VScode1.52.1版本，使用IoT link插件, 小熊派开发板）
  oc_smoke_template.c是核心代码。（包含了项目的核心任务，包含数据采集显示和任务。数据上传任务，命令下发任务，报警任务。以及完整的设计逻辑）

二、物联网三层架构每层的设计如下：
  1. 感知层：主要由烟雾传感器，火焰传感器感知环境进行数据采集，并在LCD显示屏上显示。STM32L431控制执行器（继电器），通信模组（NB-IoT模组BC35-G, GSM模组SIM800C），报警端(语音模块JQ8900，LED)执行对应的功能。
  2. 网络层：主要负责烟雾数值、报警状态值数据的上传（BC35-G）和拨打电话（SIM800C）。
  3. 应用层：华为云负责平台数据包解析，实时显示数据并推送报警信息（短信报警和华为云平台报警）。
三、本人与该项目相关的博客
  CSDN:

四、参考资料
  1. 参考了华为liteOS开源社区的帖子,社区链接：https://bbs.huaweicloud.com/forum/thread-66201-1-1.html
  2. 小熊派开发板全套资料获取： https://bbs.huaweicloud.com/forum/thread-22109-1-1.html
  3. 小熊派IoT开发板系列教程合集：https://bbs.huaweicloud.com/forum/forum.php?mod=viewthread&tid=26415
  
