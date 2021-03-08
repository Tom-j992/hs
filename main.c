/*---------------------------------------------------ͷ�ļ�������----------------------------------------------------------------*/
#include "sys.h"
#include "stm32f10x.h"									//���ļ�
#include "delay.h"											//��ʱ
#include "led.h"												//����led
#include "key.h"												//���ذ���
#include "beep.h"												//��Դ������
#include "relay.h"											//�̵���
#include "iic.h"												//IIC����
#include "oled.h"												//oled��ʾ
#include "dht11.h"											//��ʪ�ȴ�����
#include "infared.h"										//�������
#include "adc.h"												//ģ��ת��
#include "lsens.h"											//����������
#include "nec.h"												//���ⷢ�������
#include "timer.h"											//��ʱ��ͷ�ļ�
#include "usart3.h"											//��ʼ������3
#include "wifi.h"												//wifiͷ�ļ�
#include "protocol.h"
/*----------------------------------------------------����������-----------------------------------------------------------------*/
void	wifi_up_nec(u32 data);						//�ϴ��������ĺ�������

/*--------------------------------------------------ȫ�ֱ���������----------------------------------------------------------------*/

/*--------------------------------------------------------������----------------------------------------------------------------*/
int main(void)
{
  u8 key=0,lesen,timer_cnt=20,one_s_flag=0;
/*��ʼ������*/
	delay_init();													//��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	TIM2_Time_Init();											//��ʱ����ʼ��
	usart3_init(9600);									//��ʼ������3 
	LED_Init();														//ledc��ʼ��
	KEY_Init();														//������ʼ��
	BEEP_Init();													//��������ʼ��
	RELAY_Init();													//�̵�����ʼ��
	IIC_IO_Init(1);												//iic��ʼ��
	OLED_Init();													//oled��ʼ��
	INFARED_Init();												//��������ʼ��
	Lsens_Init(); 												//��ʼ������������
	NEC_OUT_Init();												//���ⷢ�ͽ��ճ�ʼ��
	NEC_IN_Init();												//���������ʼ������
	OLED_DISPLAY_CELL();									//oled����
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
						//Smart ����״̬ LED ���� �� led ��˸���û����
			break;
			case AP_STATE:
				if(cnt>=33){LED_O=!LED_O;cnt=0;}//AP ����״̬ LED ������ led ��˸���û����
				
			break;
			case WIFI_NOT_CONNECTED:
				LED_O=1;//WIFI ������ɣ� ��������·������ LED ����
			
			break;
			case WIFI_CONNECTED:
			LED_O=0;//·�������ӳɹ� LED ����
			break;
			case WIFI_CONN_CLOUD:
			LED_O=0;//wifi �������ƶ� LED ����
			default:break;
		}
			mcu_dp_bool_update(DPID_WIFI_CONTROL,wifi_flag); //BOOL�������ϱ�;
			mcu_dp_enum_update(DPID_INFRARED,!INFARED); //ö���������ϱ�;
			mcu_dp_bool_update(DPID_LEDRED,!LED_IN_R); //BOOL�������ϱ�;
			mcu_dp_bool_update(DPID_LEDBLUE,!LED_IN_G); //BOOL�������ϱ�;
			mcu_dp_bool_update(DPID_RELAY,RELAY_IN); //BOOL�������ϱ�;
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
				mcu_dp_value_update(DPID_HUMIDITY_VALUE,RX_DHT11[0]); //VALUE�������ϱ�;	
				
			} //VALUE�������ϱ�
			
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
			if(INFARED)//����
			{
					timer_cnt=20;
					if(lesen<50)
					RELAY=1;//�򿪼̵������Ƶĵ�
					else 
					RELAY=0;//�򿪼̵������Ƶĵ�
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
				
			}else if(timer_cnt>0)//���뿪����ʱ��û�ﵽ20s
			{
				if(one_s_flag==1)
				{
					timer_cnt--;
					one_s_flag=0;
					
				}
			}else//û���ҳ���20s
			{
				RELAY=0;//�ص�
				LED_R=1;
				LED_G=1;
			}
			
		}
}																	
																	
}

void wifi_up_nec(u32 data)						//��ʾʮ�������������ʾ8λ
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
	
		mcu_dp_string_update(DPID_NEC_IR,data_display,8); //STRING�������ϱ�;

}

