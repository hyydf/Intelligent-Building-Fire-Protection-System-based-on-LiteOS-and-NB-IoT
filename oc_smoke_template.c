
//更改记录：
//2022.03.05：更改命令相应处理
//2022.03.06：添加告警状态消息包

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stimer.h>
#include <osal.h>
#include <oc_lwm2m_al.h>
#include <link_endian.h>


#include "E53_SF1.h"
#include "lcd.h"
#include <usart.h>
#include <gpio.h>
#include <stm32l4xx_it.h>
#include <stm32l4xx.h>

//华为云接口配置：
#define cn_endpoint_id        "hyydf_0001"
#define cn_app_server         "119.3.250.80"
#define cn_app_port           "5683"

typedef unsigned char int8u;
typedef char int8s;
typedef unsigned short int16u;
typedef short int16s;
typedef unsigned char int24u;
typedef char int24s;
typedef int int32s;
typedef char string;
typedef char array;
typedef char varstring;
typedef char variant;

//下面为华为云编解码插件配置选项：
//1.地址域
#define cn_app_Warn 0xb
#define cn_app_Smoke 0x8
#define cn_app_response_Smoke_Control_Beep 0xa
#define cn_app_Smoke_Control_Beep 0x9
//2.数据包结构
//烟感消息：数据上报结构体
#pragma pack(1)
typedef struct
{
    int8u messageId;
    int16u Smoke_Value;
} tag_app_Smoke;
//告警状态消息，数据上报结构体
typedef struct
{
    int8u messageId;
    int16u Warn_State;
} tag_app_Warn;
//命令响应结构体
typedef struct
{
    int8u messageId;
    int16u mid;
    int8u errcode;
    int8u Beep_State;
} tag_app_Response_Smoke_Control_Beep;
//命令下发结构体
typedef struct
{
    int8u messageId;
    int16u mid;
    string Beep[3];
} tag_app_Smoke_Control_Beep;
#pragma pack()


//一些变量的定义
E53_SF1_Data_TypeDef E53_SF1_Data;
//if your command is very fast,please use a queue here--TODO
#define cn_app_rcv_buf_len 128
static int             s_rcv_buffer[cn_app_rcv_buf_len];
static int             s_rcv_datalen;
static osal_semp_t     s_rcv_sync;
uint8_t voiceopen[4] = {0xaa,0x02,0x00,0xac};
uint8_t voicecirculate[5]={0xaa,0x18,0x01,0x01,0xc4}; 
uint8_t voiceonetime[5]={0xaa,0x18,0x01,0x02,0xc5}; 
uint8_t voiceclose[4] = {0xaa,0x04,0x00,0xae};
int hand_warn = 0;//手动报警标志位
int8_t flag = 1;//标志位
int warnstate = 0;//报警状态


//use this function to push all the message to the buffer
static int app_msg_deal(void *usr_data, en_oc_lwm2m_msg_t type, void *data, int len)
{
    unsigned char *msg;
    msg = data;
    int ret = -1;

    if(len <= cn_app_rcv_buf_len)
    {
    	if (msg[0] == 0xaa && msg[1] == 0xaa)
    	{
    		printf("OC respond message received! \n\r");
    		return ret;
    	}
        memcpy(s_rcv_buffer,msg,len);
        s_rcv_datalen = len;

        (void) osal_semp_post(s_rcv_sync);

        ret = 0;

    }
    return ret;
}

//串口2、3的初始化
void uart2_3_init()
{
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    LOS_HwiCreate(USART2_IRQn, 4,0,USART2_IRQHandler,NULL);	//串口2中断优先级是4
    LOS_HwiCreate(USART3_IRQn, 4,0,USART3_IRQHandler,NULL);	//串口3的中断优先级是4
}
//串口2 
static ssize_t uart2_send(const char *buf,size_t len, unsigned int timeout)
{
    HAL_UART_Transmit(&huart2,(unsigned char *)buf,len,timeout);
    HAL_Delay(200);
    return len; 
}
//串口3
static ssize_t uart3_send(const char *buf,size_t len, unsigned int timeout)
{
    HAL_UART_Transmit(&huart3,(unsigned char *)buf,len,timeout);
    HAL_Delay(200);
    return len; 
}
//按键的外部中断
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin)
	{
		case KEY1_Pin://手动报警按键
            hand_warn = 1;//手动报警
            warnstate = 1;
			break;
		case KEY2_Pin://紧急制动取消报警
			hand_warn = 0;
            warnstate = 0;
			break;
		default:
			break;
	}
}
//报警任务，感知包含：火焰，烟感，控制包含灯，继电器，
static int app_warn_entry()
{
    while (1)
    {
        if((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12) == 0)||(((int)E53_SF1_Data.Smoke_Value) > 3000)||(hand_warn==1))
        {
            if((((int)E53_SF1_Data.Smoke_Value) > 3000))
            {
                warnstate = 2;
            }
                if((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12) == 0))
            {
                warnstate = 3;
            }
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,SET);//灯
            HAL_GPIO_WritePin(GPIOA,jidianqi_Pin,RESET);//继电器
            while(flag)
            {
                uart3_send("ATD1561288****;\r\n",16,200);//拨打电话
                uart2_send(voicecirculate,5,200);//循环模式
                //uart2_send(voiceonetime,5,200);//单曲模式
                uart2_send(voiceopen,4,200);//语音
                flag = 0;
            }
        }
        else if(hand_warn==0)
        {
            flag=1;
            warnstate = 0;
            uart3_send("ATH\r\n",5,200);
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,RESET);
            HAL_GPIO_WritePin(GPIOA,jidianqi_Pin,SET);
            uart2_send(voiceclose,4,200);
        }
        osal_task_sleep(1000);
    }
       
    return 0;
}
//命令下发时设备响应
static int app_cmd_task_entry()
{
    int ret = -1;
    tag_app_Response_Smoke_Control_Beep Response_Smoke_Control_Beep;
    tag_app_Smoke_Control_Beep *Smoke_Control_Beep;
    int8_t msgid;

    while(1)
    {
        if(osal_semp_pend(s_rcv_sync,cn_osal_timeout_forever))
        {
            msgid = s_rcv_buffer[0] & 0x000000FF;
            switch (msgid)
            {
                 case cn_app_Smoke_Control_Beep:
                    Smoke_Control_Beep = (tag_app_Smoke_Control_Beep *)s_rcv_buffer;
                    printf("Smoke_Control_Beep:msgid:%d mid:%d", Smoke_Control_Beep->messageId, ntohs(Smoke_Control_Beep->mid));
                    /********** code area for cmd from IoT cloud  **********/
                    if (Smoke_Control_Beep->Beep[0] == 'O' && Smoke_Control_Beep->Beep[1] == 'N')
                    {
                        hand_warn = 1;
                        //E53_SF1_Beep_StatusSet(ON);
                        Response_Smoke_Control_Beep.messageId = cn_app_response_Smoke_Control_Beep;
                    	Response_Smoke_Control_Beep.mid = Smoke_Control_Beep->mid;
                        Response_Smoke_Control_Beep.errcode = 0;
                		Response_Smoke_Control_Beep.Beep_State = 1;
                        oc_lwm2m_report((char *)&Response_Smoke_Control_Beep,sizeof(Response_Smoke_Control_Beep),1000);    ///< report cmd reply message
                    }
                    if (Smoke_Control_Beep->Beep[0] == 'O' && Smoke_Control_Beep->Beep[1] == 'F' && Smoke_Control_Beep->Beep[2] == 'F')
                    {
                        hand_warn = 0;
                        //E53_SF1_Beep_StatusSet(OFF);
                        Response_Smoke_Control_Beep.messageId = cn_app_response_Smoke_Control_Beep;
                    	Response_Smoke_Control_Beep.mid = Smoke_Control_Beep->mid;
                        Response_Smoke_Control_Beep.errcode = 0;
                		Response_Smoke_Control_Beep.Beep_State = 0;
                        oc_lwm2m_report((char *)&Response_Smoke_Control_Beep,sizeof(Response_Smoke_Control_Beep),1000);    ///< report cmd reply message
                    }
                    /********** code area end  **********/
                    break;
                default:
                    break;
            }
        }
    }

    return ret;
}
//数据上报
static int app_report_task_entry()
{
    int ret = -1;
    oc_config_param_t      oc_param;
    tag_app_Smoke Smoke;
    tag_app_Warn Warn;
    (void) memset(&oc_param,0,sizeof(oc_param));
    oc_param.app_server.address = cn_app_server;
    oc_param.app_server.port = cn_app_port;
    oc_param.app_server.ep_id = cn_endpoint_id;
    oc_param.boot_mode = en_oc_boot_strap_mode_factory;
    oc_param.rcv_func = app_msg_deal;
    ret = oc_lwm2m_config(&oc_param);
    if (0 != ret)
    {
    	return ret;
    }
    while(1) //--TODO ,you could add your own code here
    {
        Smoke.messageId = cn_app_Smoke;
        Smoke.Smoke_Value = htons((int)E53_SF1_Data.Smoke_Value);//烟感值
        Warn.messageId = cn_app_Warn;
        Warn.Warn_State = htons((int)warnstate);//报警状态
        oc_lwm2m_report( (char *)&Smoke, sizeof(Smoke), 1000);//发送烟感值
        oc_lwm2m_report( (char *)&Warn, sizeof(Warn), 1000);//发送报警状态值
        osal_task_sleep(2000);//2秒发一次
    }
    return ret;
}

//数据采集串口打印,显示屏显示
static int app_collect_task_entry()
{
    Init_E53_SF1();
    while (1)
    {
        E53_SF1_Read_Data();//获取烟雾传感器的数据，数据采集
        printf("\r\n******************************Smoke Value is  %d\r\n", (int)E53_SF1_Data.Smoke_Value);
        printf("\r\n******************************Warn State is  %d\r\n", warnstate);
		LCD_ShowString(10, 130, 200, 16, 24, "Smoke Value:");
		LCD_ShowNum(150, 130, (int)E53_SF1_Data.Smoke_Value, 5, 24);
        LCD_ShowString(10, 170, 200, 16, 24, "Warn State: ");
		LCD_ShowNum(160, 170, (int)warnstate, 1, 24); 
        osal_task_sleep(1000);
    }
    return 0;
}
int standard_app_demo_main()
{
    uart2_3_init(); 
    osal_int_connect(KEY1_EXTI_IRQn, 3,0,Key1_IRQHandler,NULL);
    osal_int_connect(KEY2_EXTI_IRQn, 3,0,Key2_IRQHandler,NULL);

	LCD_Clear(BLACK);
	POINT_COLOR = GREEN;
	LCD_ShowString(0, 10, 250, 20, 24, "Fire Fignting System");
	LCD_ShowString(10, 50, 200, 16, 24, "IP:");
	LCD_ShowString(80, 50, 200, 16, 24, cn_app_server);
	LCD_ShowString(10, 90, 200, 16, 24, "PORT:");
	LCD_ShowString(80, 90, 200, 16, 24, cn_app_port);

	osal_semp_create(&s_rcv_sync,1,0);


    osal_task_create("app_collect",app_collect_task_entry,NULL,0x400,NULL,10);
    osal_task_create("app_report",app_report_task_entry,NULL,0x1000,NULL,10);
    osal_task_create("app_command",app_cmd_task_entry,NULL,0x1000,NULL,10);
    osal_task_create("WARN",app_warn_entry,NULL,0x400,NULL,9);

    return 0;
}





