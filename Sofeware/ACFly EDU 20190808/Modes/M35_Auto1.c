#include "Modes.h"
#include "Basic.h"
#include "stdlib.h"
#include <stdio.h>
#include "M35_Auto1.h"

#include "AC_Math.h"
#include "Receiver.h"
#include "InteractiveInterface.h"
#include "ControlSystem.h"
#include "MeasurementSystem.h"

static void M35_Auto1_MainFunc();
static void M35_Auto1_enter();
static void M35_Auto1_exit();
const Mode M35_Auto1 = 
{
	50 , //mode frequency
	M35_Auto1_enter , //enter
	M35_Auto1_exit ,	//exit
	M35_Auto1_MainFunc ,	//mode main func
};

typedef struct
{
	//�˳�ģʽ������
	uint16_t exit_mode_counter;
	
	//�Զ�����״̬��
	uint8_t auto_step1;	//0-��¼��ťλ��
											//1-�ȴ���ť������� 
											//2-�ȴ������� 
											//3-�ȴ�2��
											//4-����
											//5-�ȴ��������
	uint16_t auto_counter;
	float last_button_value;
	
	float last_height;
}MODE_INF;
static MODE_INF* Mode_Inf;

static void M35_Auto1_enter()
{
	Led_setStatus( LED_status_running1 );
	
	//��ʼ��ģʽ����
	Mode_Inf = malloc( sizeof( MODE_INF ) );
	Mode_Inf->exit_mode_counter = 0;
	Mode_Inf->auto_step1 = Mode_Inf->auto_counter = 0;
	Altitude_Control_Enable();
}

static void M35_Auto1_exit()
{
	Altitude_Control_Disable();
	Attitude_Control_Disable();
	
	free( Mode_Inf );
}


/******���ǵĹ�һ������ʵ��******/
static void M35_Auto1_MainFunc()
{
	/*�ⲿ������ȡ*/
	extern uint8_t Uart0_mode_contrl;//������ģʽ���ݷ���
	extern uint8_t SDI_mode_contrl;//openmv����ģʽ�޸�
	
	extern uint16_t uart0_msg_pack[31];//����վ�������ݽ���
	extern uint8_t uart0_num;//���ε����
	
	extern uint8_t SDI_area_central[11];//openmv����������ĵ�����
	extern uint8_t SDI_blob_central[3];//openmvɫ�����ĵ�����
	
	extern uint16_t UWB_data;//UWB����
	
	extern float Primitive_Height;//������ԭʼ�߶ȡ��������˻��Եظ߶�
	/*�ⲿ������ȡ*/
	
	/*������*/
	static float SDI_fly_1 = 0.0f;//
	static float SDI_fly_2 = 0.0f;//
	static float SDI_fly_3 = 1.0f;//
	static float SDI_fly_4 = 0.0f;//
	static float SDI_fly_5 = 0.0f;//
	static float SDI_fly_6 = 0.0f;//
	static float SDI_fly_7 = 0.0f;//
	static float SDI_fly_8 = 0.0f;//
	static float SDI_fly_9 = 0.0f;//
	static float SDI_fly_10 = 0.0f;//
	
	
	static float M35_init_yaw = 0.0f;//���ƫ����
	static float M35_last_yaw = 0.0f;//��һ��ƫ����
	static float M35_now_yaw = 0.0f;//���ڵ�ƫ����
	
	
	char oled_srr[5];//OLED��ʾת��
	/*������*/
	
	/*Զ�̲�������*/
	if(uart0_msg_pack[0] == 1)//Ѳ��ģʽ��������
	{
		uart0_msg_pack[0] = 0;//��ֵ��������
		switch(uart0_num)
		{
			case 1:
				SDI_fly_1 = uart0_msg_pack[1]/100.0f;
			break;
			case 2:
				SDI_fly_2 = uart0_msg_pack[2]/100.0f;
			break;
			case 3:
				SDI_fly_3 = uart0_msg_pack[3]/100.0f;
			break;
			case 4:
				SDI_fly_4 = uart0_msg_pack[4]/100.0f;
			break;
			case 5:
				SDI_fly_5 = uart0_msg_pack[5]/100.0f;
			break;
			case 6:
				SDI_fly_6 = uart0_msg_pack[6]/100.0f;
			break;
			case 7:
				SDI_fly_7 = uart0_msg_pack[7]/100.0f; 
			break;
			case 8:
				SDI_fly_8 = uart0_msg_pack[8]/100.0f;
			break;
			case 9:
				SDI_fly_9 = uart0_msg_pack[9]/100.0f;
			break;
			case 10:
				SDI_fly_10 = uart0_msg_pack[10]/100.0f;
			break;
			default:;
		}
		
		OLED_Clear();
		OLED_Draw_Str8x6( "M_1:" , 2 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_1 );
		OLED_Draw_Str8x6( oled_srr , 2 , 30 );
	
		OLED_Draw_Str8x6( "M_2:" , 3 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_2 );
		OLED_Draw_Str8x6( oled_srr , 3 , 30 );
	
		OLED_Draw_Str8x6( "M_3:" , 4 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_3 );
		OLED_Draw_Str8x6( oled_srr , 4 , 30 );
	
		OLED_Draw_Str8x6( "M_4:" , 5 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_4 );
		OLED_Draw_Str8x6( oled_srr , 5 , 30 );
	
		OLED_Draw_Str8x6( "M_5:" , 6 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_5 );
		OLED_Draw_Str8x6( oled_srr , 6 , 30 );
	
		OLED_Draw_Str8x6( "M_6:" , 7 , 0 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_6 );
		OLED_Draw_Str8x6( oled_srr , 7 , 30 );
	
		OLED_Draw_Str8x6( "M_7:" , 2 , 64 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_7 );
		OLED_Draw_Str8x6( oled_srr , 2 , 94 );
	
		OLED_Draw_Str8x6( "M_8:" , 3 , 64 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_8 );
		OLED_Draw_Str8x6( oled_srr , 3 , 94 );
	
		OLED_Draw_Str8x6( "M_9:" , 4 , 64 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_9 );
		OLED_Draw_Str8x6( oled_srr , 4 , 94 );
		
		OLED_Draw_Str8x6( "M_10:" , 5 , 64 );
		sprintf( oled_srr , "%5.2f" , SDI_fly_10 );
		OLED_Draw_Str8x6( oled_srr , 5 , 94 );
	}
	/*Զ�̲�������*/
	
	/*test��*/
	//��ʾ��ǰ����״̬
	static uint8_t SDK_auto_step1 = 0;
	if(Mode_Inf->auto_step1 != 0)SDK_auto_step1 = Mode_Inf->auto_step1;
	OLED_Draw_Str8x6( "c" , 0 , 90 );
	sprintf( oled_srr , "%3d" , SDK_auto_step1);
	OLED_Draw_Str8x6( oled_srr , 0 , 100 );
	
	//��ȡ��ǰ������̬��Ϣ
	Quaternion _M35Auto = get_attitude();
	
	
	OLED_Update();
	/*test��*/
	
	
	const Receiver* rc = get_current_Receiver();
		
	if( rc->available == false )
	{
		//���ջ�������
		//����
		Position_Control_set_XYLock();
		Position_Control_set_TargetVelocityZ( -50 );
		return;
	}
	float throttle_stick = rc->data[0];
	float yaw_stick = rc->data[1];
	float pitch_stick = rc->data[2];
	float roll_stick = rc->data[3];	
	
	/*�ж��˳�ģʽ*/
		if( throttle_stick < 5 && yaw_stick < 5 && pitch_stick < 5 && roll_stick > 95 )
		{
			if( ++Mode_Inf->exit_mode_counter >= 50 )
			{
				change_Mode( 1 );
				return;
			}
		}
		else
			Mode_Inf->exit_mode_counter = 0;
	/*�ж��˳�ģʽ*/
		
	//�ж�ҡ���Ƿ����м�
	bool sticks_in_neutral = 
		in_symmetry_range_offset_float( throttle_stick , 5 , 50 ) && \
		in_symmetry_range_offset_float( yaw_stick , 5 , 50 ) && \
		in_symmetry_range_offset_float( pitch_stick , 5 , 50 ) && \
		in_symmetry_range_offset_float( roll_stick , 5 , 50 );
	
	
//	
//	if( sticks_in_neutral && get_Position_Measurement_System_Status() == Measurement_System_Status_Ready )
	if( sticks_in_neutral && get_Altitude_Measurement_System_Status() == Measurement_System_Status_Ready )
	{
		//ҡ�����м�
		//ִ���Զ�����		
		//ֻ����λ����Чʱ��ִ���Զ�����
		
		//��ˮƽλ�ÿ���
		Position_Control_Disable();//���޸ġ��ر�ˮƽ����
		switch( Mode_Inf->auto_step1 )
		{
			case 0:
			{
				Uart0_mode_contrl = 1;//�رռ���
				M35_init_yaw = Quaternion_getYaw(_M35Auto);//��¼���ǰ�ĵ�������ϵƫ����
				Mode_Inf->last_button_value = rc->data[5];
				++Mode_Inf->auto_step1;
				Mode_Inf->auto_counter = 0;
			}break;
			
			case 1:
			{//�ȴ���ť�������
				if( get_is_inFlight() == false && fabsf( rc->data[5] - Mode_Inf->last_button_value ) > 15 )
				{
					Mode_Inf->last_button_value = rc->data[5];//��¼��ť��ʼ״̬
					Position_Control_Takeoff_HeightRelative( 90.0f );
					++Mode_Inf->auto_step1;
					Mode_Inf->auto_counter = 0;
				}
			}break;
				
			case 2:
			{//�ȴ�������
				if( get_Altitude_ControlMode() == Position_ControlMode_Position )
				{
					Uart0_mode_contrl = 2;//��������
					
					Mode_Inf->auto_step1 = 3;//�߶Ƚ���
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
			case 3:
			{
				Position_Control_set_TargetVelocityZ(90-Primitive_Height);//���Ƹ߶ȣ�ά����100cm
				
				if(fabs(90-Primitive_Height) < 5)//�߶Ƚ������
				{
					Position_Control_set_TargetVelocityZ(0);
					Position_Control_set_ZLock();//���߶�
					Mode_Inf->auto_step1 = 10;//���빤��״̬
				}
				
				if(fabsf( rc->data[5] - Mode_Inf->last_button_value ) > 15 )
				{
				 	Mode_Inf->last_button_value = rc->data[5];//��¼��ǰ��ť״̬
					Mode_Inf->auto_step1 = 200;//�½��������£�����״̬ 0
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
			
			
			
			
	/*******
			case 10:
			{
				Position_Control_set_TargetPositionXYRelativeBodyHeading(130,0);
				++Mode_Inf->auto_step1;
				if(fabsf( rc->data[5] - Mode_Inf->last_button_value ) > 15 )
				{
				 	Mode_Inf->last_button_value = rc->data[5];//��¼��ǰ��ť״̬
					Mode_Inf->auto_step1 = 200;//�½��������£�����״̬ 0
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
			case 11:
			{
				Attitude_Control_set_Target_YawRelative(degree2rad(90));
				++Mode_Inf->auto_step1;
				if(fabsf( rc->data[5] - Mode_Inf->last_button_value ) > 15 )
				{
				 	Mode_Inf->last_button_value = rc->data[5];//��¼��ǰ��ť״̬
					Mode_Inf->auto_step1 = 200;//�½��������£�����״̬ 0
					Mode_Inf->auto_counter = 0;
				}
			}
			
			case 12:
			{
				if( get_Position_ControlMode() == Position_ControlMode_Position )
				{
					++Mode_Inf->auto_step1;
					Mode_Inf->auto_counter = 0;
				}
				
				if(fabsf( rc->data[5] - Mode_Inf->last_button_value ) > 15 )
				{
				 	Mode_Inf->last_button_value = rc->data[5];//��¼��ǰ��ť״̬
					Mode_Inf->auto_step1 = 200;//�½��������£�����״̬ 0
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
********************************/
			
			case 200:
			{//����
				//Position_Control_set_XYLock();���޸ġ��ر�ˮƽ����
				Attitude_Control_set_YawLock(); 
				++Mode_Inf->auto_step1;
			}break;		
				
			case 201:
			{//�ȴ�λ���������
				if( get_Altitude_ControlMode() == Position_ControlMode_Position )
				{
					Position_Control_set_TargetVelocityZ( -40 );
					++Mode_Inf->auto_step1;
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
			/******�����½�״̬���жϸ߶�ʹ��ƽ������************************************/			
			case 202:
			{
				//Position_Control_set_XYLock();���޸ġ��ر�ˮƽ����
				Attitude_Control_set_YawLock();
				Position_Control_set_TargetVelocityZ( -50 );//���ʣ��Ƿ���Ҫ��Ӵ˾�
				if(Primitive_Height<50)
				{
					Position_Control_set_TargetVelocityZ( -10 );
					++Mode_Inf->auto_step1;
					Mode_Inf->auto_counter = 0;
				}
			}break;
			
			/******************************************/
			case 203:
			{
				//�ȴ��������
				if( get_is_inFlight() == false )
				{
					Mode_Inf->auto_step1 = 0;
					Mode_Inf->auto_counter = 0;
				}
			}break;
			default:break;
		}
	}
	else
	{
		ManualControl:
		//ҡ�˲����м�
		//�ֶ�����
		Mode_Inf->auto_step1 = Mode_Inf->auto_counter = 0;
		
		
		//�ر�ˮƽλ�ÿ���
		Position_Control_Disable();
		
		//�߶ȿ�������
		if( in_symmetry_range_offset_float( throttle_stick , 5 , 50 ) )
			Position_Control_set_ZLock();
		else
		{
			float throttle_stick_test=throttle_stick - 50.0f;
			if(throttle_stick_test>0) throttle_stick_test=0;
			
			Position_Control_set_TargetVelocityZ( ( throttle_stick_test )* 6 );
		}
		//ƫ����������
		if( in_symmetry_range_offset_float( yaw_stick , 5 , 50 ) )
			Attitude_Control_set_YawLock();
		else
			Attitude_Control_set_Target_YawRate( ( 50.0f - yaw_stick )*0.05f );
		
		//Roll Pitch��������
		Attitude_Control_set_Target_RollPitch( \
			( roll_stick 	- 50.0f )*0.015f, \
			( pitch_stick - 50.0f )*0.015f );
	}
}

