/*---------------------------------------------------头文件定义区----------------------------------------------------------------*/
#include "sys.h"
#include "stm32f10x.h"									//库文件
#include "delay.h"											//延时
#include "led.h"												//板载led
#include "key.h"												//板载按键
#include "beep.h"												//无源蜂鸣器
#include "relay.h"											//继电器
#include "iic.h"												//IIC总线
#include "oled.h"												//oled显示
#include "dht11.h"											//温湿度传感器
#include "infared.h"										//人体红外
#include "adc.h"												//模数转换
#include "lsens.h"											//光敏传感器
#include "nec.h"												//红外发送与接收
#include "timer.h"											//定时器头文件
#include "usart3.h"											//初始化串口3
#include "wifi.h"												//wifi头文件
#include "protocol.h"
/*----------------------------------------------------函数声明区-----------------------------------------------------------------*/
void	wifi_up_nec(u32 data);						//上传解码过后的红外数据

/*--------------------------------------------------全局变量声明区----------------------------------------------------------------*/

/*--------------------------------------------------------主函数----------------------------------------------------------------*/
int main(void)
{
  u8 key=0,lesen,timer_cnt=20,one_s_flag=0;
/*初始化函数*/
	delay_init();													//延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	TIM2_Time_Init();											//定时器初始化
	usart3_init(9600);									//初始化串口3 
	LED_Init();														//ledc初始化
	KEY_Init();														//按键初始化
	BEEP_Init();													//蜂鸣器初始化
	RELAY_Init();													//继电器初始化
	IIC_IO_Init(1);												//iic初始化
	OLED_Init();													//oled初始化
	INFARED_Init();												//人体红外初始化
	Lsens_Init(); 												//初始化光敏传感器
	NEC_OUT_Init();												//红外发送接收初始化
	NEC_IN_Init();												//红外输入初始化函数
	OLED_DISPLAY_CELL();									//oled清屏
	wifi_protocol_init();
	while(1)
	{

		key=KEY_Scanf();
    switch(key)
    {
      case 1:wifi_flag=!wifi_flag;break;
      case 2:NEC_Decoding();wifi_up_nec(Nec_Flag<<24|Mec_Code[0]<<16|Mec_Code[1]<<8|Mec_Code[2]); break;
    
    }
		switch(mcu_get_wifi_work_state())
		{
			case SMART_CONFIG_STATE:
				if(cnt>=13){LED_O=!LED_O;cnt=0;}
						//Smart 配置状态 LED 快闪 ， led 闪烁请用户完成
			break;
			case AP_STATE:
				if(cnt>=33){LED_O=!LED_O;cnt=0;}//AP 配置状态 LED 慢闪， led 闪烁请用户完成
				
			break;
			case WIFI_NOT_CONNECTED:
				LED_O=1;//WIFI 配置完成， 正在连接路由器， LED 常暗
			
			break;
			case WIFI_CONNECTED:
			LED_O=0;//路由器连接成功 LED 常亮
			break;
			case WIFI_CONN_CLOUD:
			LED_O=0;//wifi 已连上云端 LED 常亮
			default:break;
		}
			mcu_dp_bool_update(DPID_WIFI_CONTROL,wifi_flag); //BOOL型数据上报;
			mcu_dp_enum_update(DPID_INFRARED,!INFARED); //枚举型数据上报;
			mcu_dp_bool_update(DPID_LEDRED,!LED_IN_R); //BOOL型数据上报;
			mcu_dp_bool_update(DPID_LEDBLUE,!LED_IN_G); //BOOL型数据上报;
			mcu_dp_bool_update(DPID_RELAY,RELAY_IN); //BOOL型数据上报;
			if(dht11_cnt%25==0)
			{
				lesen=Lsens_Get_Val();
				mcu_dp_value_update(DPID_LIGHT_LESEN,lesen); 
			}
			if(dht11_cnt>=120)
			{
				DHT11_R_data();
				dht11_cnt=0;
				one_s_flag=1;
				OLED_CTS_DISPLAY(RX_DHT11[2]*100+RX_DHT11[3],RX_DHT11[0]);  
				mcu_dp_value_update(DPID_TEMP_CURRENT,RX_DHT11[2]*10+RX_DHT11[3]);
				mcu_dp_value_update(DPID_HUMIDITY_VALUE,RX_DHT11[0]); //VALUE型数据上报;	
				
			} //VALUE型数据上报
			
		wifi_uart_service();
				
		if(wifi_flag) //
		{	
			OLED_DISPLAY_8X16_string(0,0,"wifi");
			
		}
		else
		{
			OLED_DISPLAY_8X16_string(0,0,"auto");
			OLED_DISPLAY_8X16(0,13,timer_cnt/10+48);
			OLED_DISPLAY_8X16(0,14,timer_cnt%10+48);
			if(INFARED)//有人
			{
					timer_cnt=20;
					if(lesen<50)
					RELAY=1;//打开继电器控制的灯
					else 
					RELAY=0;//打开继电器控制的灯
					if((RX_DHT11[0]<rh_min)||(RX_DHT11[0]>rh_max))
					{
						LED_G=0;
					}
					else
					{
						LED_G=1;
					}
					if(((RX_DHT11[2]*10+RX_DHT11[3])<temp_min)||((RX_DHT11[2]*10+RX_DHT11[3])>temp_max))
					{
						LED_R=0;
					}
					else
					{
						LED_R=1;
					}
					if((LED_IN_R|LED_IN_G)==0)
					{
							BEEP_1(1);
					}
				
			}else if(timer_cnt>0)//人离开但是时间没达到20s
			{
				if(one_s_flag==1)
				{
					timer_cnt--;
					one_s_flag=0;
					
				}
			}else//没人且超过20s
			{
				RELAY=0;//关灯
				LED_R=1;
				LED_G=1;
			}
			
		}
}																	
																	
}

void wifi_up_nec(u32 data)						//显示十六进制数最多显示8位
{
	
	u8 y,tmp,data_display[9];
	u32 data2=data;
	for(y=0;y<8;y++)
	{
				tmp=data2&0xf;
				if(tmp<10)
				{
					data_display[7-y]=tmp+'0';
					
				}else
				{
					data_display[7-y]=tmp+'A'-10;
				}
				data2>>=4;
	}
	data_display[8]='\0';
	
		mcu_dp_string_update(DPID_NEC_IR,data_display,8); //STRING型数据上报;

}

