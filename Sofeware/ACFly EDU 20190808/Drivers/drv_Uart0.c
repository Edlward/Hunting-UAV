#include "Basic.h"
#include "drv_Uart0.h"

#include "Quaternion.h"
#include "MeasurementSystem.h"

#include "STS.h"
#include "Sensors_Backend.h"
#include "RingBuf.h"

#include "TM4C123GH6PM.h"
#include "uart.h"
#include "sysctl.h"
#include "gpio.h"
#include "pin_map.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_gpio.h"
#include "Timer.h"
#include "udma.h"

#define Uart0_BUFFER_SIZE 64

//�����ж�
static void UART0_Handler();

/*���ͻ�����*/
	static uint8_t Uart0_tx_buffer[Uart0_BUFFER_SIZE];
	static RingBuf_uint8_t Uart0_Tx_RingBuf;
/*���ͻ�����*/

/*���ջ�����*/
	static uint8_t Uart0_rx_buffer[Uart0_BUFFER_SIZE];
	static RingBuf_uint8_t Uart0_Rx_RingBuf;
/*���ջ�����*/

static bool Uart0_RCTrigger( unsigned int Task_ID );
static void Uart0_Server( unsigned int Task_ID );

static bool Uart0_TCTrigger( unsigned int Task_ID );
static void Uart0_Server_Send( unsigned int Task_ID );

void init_drv_Uart0()
{
	//ʹ��Uart0���ţ�Rx:PA0 Tx:PA1��
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	//ʹ��UART0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	
	//����GPIO
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIOA_BASE, GPIO_PIN_0 | GPIO_PIN_1);//GPIO��UARTģʽ����
		
	//����Uart
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet() , 115200,			
	                   (UART_CONFIG_WLEN_8 
										| UART_CONFIG_STOP_ONE 
										|	UART_CONFIG_PAR_NONE));	
	
	//��ʼ��������
	RingBuf_uint8_t_init( &Uart0_Tx_RingBuf , Uart0_tx_buffer , Uart0_BUFFER_SIZE );
	RingBuf_uint8_t_init( &Uart0_Rx_RingBuf , Uart0_rx_buffer , Uart0_BUFFER_SIZE );
	
	//���ô��ڽ����ж�
	UARTIntEnable( UART0_BASE , UART_INT_RX | UART_INT_RT);
	UARTIntRegister( UART0_BASE , UART0_Handler );
	
	//����DMA����
	uDMAChannelControlSet( UDMA_PRI_SELECT | UDMA_CH9_UART0TX , \
		UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1 );
	UARTDMAEnable( UART0_BASE , UART_DMA_TX );
	UARTIntRegister( UART0_BASE , UART0_Handler );	
	uDMAChannelAssign(UDMA_CH9_UART0TX  );	
	
	//���ж�
	IntPrioritySet( INT_UART0 , INT_PRIO_7 );
	IntEnable( INT_UART0 );
	
	
//	//ע�ᴫ����
//	PositionSensorRegister( 3 , Position_Sensor_Type_RelativePositioning , Position_Sensor_DataType_s_xy , Position_Sensor_frame_ENU , 0.1f , false );
	//��Ӽ򵥶��ο���Э���������
	STS_Add_Task( STS_Task_Trigger_Mode_Custom , 0 , Uart0_RCTrigger , Uart0_Server );//����������ӣ�ֻ���յ�������Ϣ�Żᱻ����
	STS_Add_Task( STS_Task_Trigger_Mode_Custom , 0 , Uart0_TCTrigger , Uart0_Server_Send );//����������ӣ��� M35 ���ƣ�Uart0_mode_contrl��
}



uint8_t Uart0_mode_contrl = 0;//״̬�ⲿ��������ƶ๦��ִ�ж�
uint8_t Uart0_mode_flag = 0;//�ݴ�Uart0_mode_contrl��ֵ
static bool Uart0_TCTrigger( unsigned int Task_ID )
{//���ʹ�������
	if( Uart0_mode_contrl != 0 )
	{
		Uart0_mode_flag = Uart0_mode_contrl;
		Uart0_mode_contrl = 0;//ִ��һ�η������񼴹رշ���
		return true;
	}
	return false;
}


static void Uart0_Server_Send( unsigned int Task_ID )
{//���ͷ�����
	static uint8_t tc_buf[7] = {0xAA,0x55,0,0,0,0,0};
	
	extern float Primitive_Height;//������ԭʼ�߶ȣ���ϽǶȼ���
	
	uint16_t fly_Height = (uint16_t)Primitive_Height;
	
	if(Uart0_mode_flag > 0 && Uart0_mode_flag < 9)
	{
		tc_buf[2] = 0x1f;//ģʽ������31ģʽ ���ڿ��������嶯��ִ�У�
		tc_buf[3] = 1;//���ݳ���λ
		tc_buf[4] = Uart0_mode_flag;
		tc_buf[5] = tc_buf[0] + tc_buf[1] + tc_buf[2] + tc_buf[3] + tc_buf[4];
		Uart0_Send(tc_buf,6);//����
		Uart0_mode_flag = 0;
	}
	else if(Uart0_mode_flag == 33)
	{
		tc_buf[2] = 0x21;//ģʽ������33 ģʽ ���ڽ�Ŀǰ���˻��ĸ߶���Ϣ���͸�������
		tc_buf[3] = 2;//���ݳ���λ
		tc_buf[4] = fly_Height >> 8;
		tc_buf[5] = fly_Height;
		tc_buf[6] = tc_buf[0] + tc_buf[1] + tc_buf[2] + tc_buf[3] + tc_buf[4] + tc_buf[5];
		Uart0_Send(tc_buf,7);//����
		Uart0_mode_flag = 0;
	}
}


uint16_t UWB_data = 0;//UWB �ľ���
uint16_t uart0_msg_pack[31] = {0};
uint8_t uart0_num = 0;
static void Uart0_TX_Server(uint8_t mode)//���ڲ�ѯ����ר��
{
	uint8_t tc_buf[8] = {0};
	uint8_t tc_cnt = 0;
	uint8_t sum = 0;
	uint16_t tc_data_buf = 0;
	
	tc_buf[0] = 0xaa;//��ͷ1
	tc_buf[1] = 0x55;//��ͷ2
	
	tc_buf[2] = mode;//ģʽλ
	
	switch(mode)
	{
		case 1:
		{//��Ѳ��ģʽ��ز���
			tc_buf[3] = 3;//���ݳ���
			for(tc_cnt = 0;tc_cnt < 10;tc_cnt ++)
			{
				tc_buf[4] = tc_cnt + 1;//�������
				
				tc_data_buf = uart0_msg_pack[1+tc_cnt];//��ȡ����ֵ	��uart0_msg_pack[1]��ʼ
				
				tc_buf[5] = tc_data_buf >> 8;//���͸߰�λ
				tc_buf[6] = tc_data_buf;//���͵Ͱ�λ
				
				sum = tc_buf[0] + tc_buf[1] + tc_buf[2] + tc_buf[3] + tc_buf[4] + tc_buf[5] + tc_buf[6];
				tc_buf[7] = sum;
				
				Uart0_Send(tc_buf,8);//����
			}
		}break;
		
		case 2:
		{//����Ѱɫ��ģʽ��ز���
			tc_buf[3] = 3;//���ݳ���
			for(tc_cnt = 0;tc_cnt < 10;tc_cnt ++)
			{
				tc_buf[3] = 3;//���ݳ���
				for(tc_cnt = 0;tc_cnt < 10;tc_cnt ++)
				{
					tc_buf[4] = tc_cnt + 1;//�������
					
					tc_data_buf = uart0_msg_pack[11+tc_cnt];//��ȡ����ֵ	��uart0_msg_pack[11]��ʼ
					
					tc_buf[5] = tc_data_buf >> 8;//���͸߰�λ
					tc_buf[6] = tc_data_buf;//���͵Ͱ�λ
				
					sum = tc_buf[0] + tc_buf[1] + tc_buf[2] + tc_buf[3] + tc_buf[4] + tc_buf[5] + tc_buf[6];
					tc_buf[7] = sum;
				
					Uart0_Send(tc_buf,8);//����
				}
			}
		}break;
		
		case 3:
		{//���ش���ģʽ��ز���
			for(tc_cnt = 0;tc_cnt < 10;tc_cnt ++)
			{
				tc_buf[3] = 3;//���ݳ���
				for(tc_cnt = 0;tc_cnt < 10;tc_cnt ++)
				{
					tc_buf[4] = tc_cnt + 1;//�������
					
					tc_data_buf = uart0_msg_pack[21+tc_cnt];//��ȡ����ֵ	��uart0_msg_pack[21]��ʼ
					
					tc_buf[5] = tc_data_buf >> 8;//���͸߰�λ
					tc_buf[6] = tc_data_buf;//���͵Ͱ�λ
				
					sum = tc_buf[0] + tc_buf[1] + tc_buf[2] + tc_buf[3] + tc_buf[4] + tc_buf[5] + tc_buf[6];
					tc_buf[7] = sum;
				
					Uart0_Send(tc_buf,8);//����
				}
			}
		}break;
		default:;
	}
}


static bool Uart0_RCTrigger( unsigned int Task_ID )
{//���������崮�����ݵĴ�������
	if( Uart0_DataAvailable() )
		return true;
	return false;
}


static void Uart0_Server( unsigned int Task_ID )
{
	//Զ�̵���Э�����
	
	/*״̬������*/
		static uint8_t rc_step1 = 0;	//0�����հ�ͷ'0xAA' '0X55'
																	//1������1�ֽ���Ϣ���
																	//2������1�ֽ���Ϣ����
																	//3���������ݰ�����
																	//4������1�ֽ�У��
		static uint8_t rc_step2 = 0;
	
		#define MAX_SDI_PACKET_SIZE 30
		static uint8_t msg_type;//ģʽ����
		static uint8_t msg_nume;//�������
		static uint8_t msg_length;//���ݳ���
		static uint8_t msg_pack[MAX_SDI_PACKET_SIZE];//������
		static uint8_t sum;//У���
		
		#define reset_SDI_RC ( rc_step1 = rc_step2 = 0 )//״̬��λ
	/*״̬������*/
	
	
	uint8_t rc_buf[20];
	uint8_t length = read_Uart0( rc_buf , 20 );//��ȡ��������
	for( uint8_t i = 0 ; i < length ; ++i )
	{
		uint8_t r_data = rc_buf[i];
		
		switch(rc_step1)
		{
			case 0:
			{
				reset_SDI_RC;//״̬��λ
				sum = 0;//У�������
				msg_type = 0;//ģʽ��������
				msg_length = 0;//���ݳ�������
				if(r_data == 0xAA)//У���ͷ1 ����0xaa
				{
					rc_step1 ++;
					sum += r_data;
				}
			}break;
			
			case 1:
			{
				if(r_data == 0x55)//У���ͷ2 ���� 0x55
				{
					rc_step1 ++;
					sum += r_data;
				}
				else reset_SDI_RC;
			}break;
			
			case 2:
			{
				if(r_data <= 255 && r_data > 0)//ֻ���� 1~255 д��ģʽ��9 Ϊ��ѯģʽ
				{
					msg_type = r_data;//����ģʽ����
					rc_step1 ++;
					sum += r_data;
				}
				else reset_SDI_RC;
			}break;
			
			case 3:
			{
				if(r_data < 12 && r_data > 0)//�޶����ݽ��ճ���
				{
					msg_length = r_data;//�������ݳ���λ
					
					rc_step1 ++;
					rc_step2 = 0;//׼�����գ���msg_pack[0]��ʼ
					sum += r_data;
				}
				else reset_SDI_RC;
			}break;
			
			case 4:
			{
				if(rc_step2 < msg_length)
				{
					msg_pack[ rc_step2 ++] = r_data;//��������0λ��ʼ������
					sum += r_data;
					if(rc_step2 == msg_length)
					{
						if((msg_type == 1) || (msg_type == 2) || (msg_type == 3))
						{
							rc_step1 = 10;//������ϣ���ת����Ӧģʽ״̬;
						}
						else if(msg_type == 9)//��ѯģʽ
						{
							rc_step1 = 11;
						}
						else if(msg_type == 33)//UWB����
						{
							rc_step1 = 12;
						}
						else reset_SDI_RC;
					}
				}
			}break;
			
			
			
			
			case 10:
			{//Ѳ��ģʽ��������ģʽ
				if(sum == r_data && msg_pack[0] < 11 && msg_pack[0] > 0)//У�����ȷ���������������Χ�ڣ��������ݴ���
				{
						uart0_msg_pack[0] = msg_type;//���ģʽ����
						uart0_msg_pack[msg_pack[0] + (msg_type - 1)*10] = ((msg_pack[1]<<8) | msg_pack[2]);//�����Ӧ���������Ч����
						uart0_num = msg_pack[0];//��������
				}
				
				reset_SDI_RC;//��������
			}break; 
			
			
			
			case 11:
			{//��ѯ����ģʽ
				if(sum == r_data)
				{
					Uart0_TX_Server(msg_pack[0]);//���ز���ģʽ
				}
				reset_SDI_RC;//��������
			}break;
			
			
			
			case 12:
			{//UWB����
				if(sum == r_data)
				{
					UWB_data = (msg_pack[0] << 8) | msg_pack[1];
				}
				reset_SDI_RC;//��������
			}break;
			
			default:;
		}
	}
	
}





void Uart0_Send( const uint8_t* data , uint16_t length )
{
	IntDisable( INT_UART0 );
	
	//��ȡʣ��Ļ������ռ�
	int16_t buffer_space = RingBuf_uint8_t_get_Freesize( &Uart0_Tx_RingBuf );
	//��ȡDMA�д����͵��ֽ���
	int16_t DMA_Remain = uDMAChannelSizeGet( UDMA_CH9_UART0TX );
	
	//����Ҫ���͵��ֽ���
	int16_t max_send_count = buffer_space - DMA_Remain;
	if( max_send_count < 0 )
		max_send_count = 0;
	uint16_t send_count = ( length < max_send_count ) ? length : max_send_count;
	
	//���������ֽ�ѹ�뻺����
	RingBuf_uint8_t_push_length( &Uart0_Tx_RingBuf , data , send_count );
//	for( uint8_t i = 0 ; i < send_count ; ++i )
//		RingBuf_uint8_t_push( &Uart0_Tx_RingBuf , data[i] );
	
	//��ȡDMA�����Ƿ����
	if( uDMAChannelIsEnabled( UDMA_CH9_UART0TX ) == false )
	{
		//DMA�����
		//���Լ�������
		uint16_t length;
		uint8_t* p = RingBuf_uint8_t_pop_DMABuf( &Uart0_Tx_RingBuf , &length );
		if( length )
		{
			uDMAChannelTransferSet( UDMA_PRI_SELECT | UDMA_CH9_UART0TX , \
				UDMA_MODE_BASIC , p , (void*)&UART0->DR , length );
			uDMAChannelEnable( UDMA_CH9_UART0TX );
		}
	}
	IntEnable( INT_UART0 );
}
static void UART0_Handler()
{
	UARTIntClear( UART0_BASE , UART_INT_OE );
	UARTRxErrorClear( UART0_BASE );
	while( ( UART0->FR & (1<<4) ) == false	)
	{
		//����
		uint8_t rdata = UART0->DR;
		RingBuf_uint8_t_push( &Uart0_Rx_RingBuf , rdata );
	}
	
	if( uDMAChannelIsEnabled( UDMA_CH9_UART0TX ) == false )
	{
		uint16_t length;
		uint8_t* p = RingBuf_uint8_t_pop_DMABuf( &Uart0_Tx_RingBuf , &length );
		if( length )
		{
			uDMAChannelTransferSet( UDMA_PRI_SELECT | UDMA_CH9_UART0TX , \
				UDMA_MODE_BASIC , p , (void*)&UART0->DR , length );
			uDMAChannelEnable( UDMA_CH9_UART0TX );
		}
	}
}
uint16_t read_Uart0( uint8_t* data , uint16_t length )
{
	IntDisable( INT_UART0 );
	uint8_t read_bytes = RingBuf_uint8_t_pop_length( &Uart0_Rx_RingBuf , data , length );
	IntEnable( INT_UART0 );
	return read_bytes;
}
uint16_t Uart0_DataAvailable()
{
	IntDisable( INT_UART0 );
	uint16_t bytes2read = RingBuf_uint8_t_get_Bytes2read( &Uart0_Rx_RingBuf );
	IntEnable( INT_UART0 );
	return bytes2read;
}