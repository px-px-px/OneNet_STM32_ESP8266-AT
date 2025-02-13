/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	onenet.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2017-05-08
	*
	*	�汾�� 		V1.1
	*
	*	˵���� 		��onenetƽ̨�����ݽ����ӿڲ�
	*
	*	�޸ļ�¼��	V1.0��Э���װ�������ж϶���ͬһ���ļ������Ҳ�ͬЭ��ӿڲ�ͬ��
	*				V1.1���ṩͳһ�ӿڹ�Ӧ�ò�ʹ�ã����ݲ�ͬЭ���ļ�����װЭ����ص����ݡ�
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f1xx.h"

//�����豸
#include "esp8266.h"

//Э���ļ�
#include "onenet.h"
#include "mqttkit.h"

//�㷨
#include "base64.h"
#include "hmac_sha1.h"

//Ӳ������
#include "usart.h"
#include "delay.h"
#include "./BSP/LED/LED.h"
#include "./BSP/DHT11/dht11.h"
#include "./BSP/TIMER/btim.h"

//C��
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"


#define PROID			"{��ƷID}"

#define ACCESS_KEY		"{��Ʒ��Կ}"

#define DEVICE_NAME		"{�豸����}"


char devid[16];

char key[48];


extern unsigned char esp8266_buf[512];

void Interaction_Succeed(void);
void Interaction_Failure(void);


/*
************************************************************
*	�������ƣ�	OTA_UrlEncode
*
*	�������ܣ�	sign��Ҫ����URL����
*
*	��ڲ�����	sign�����ܽ��
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		+			%2B
*				�ո�		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	�������ƣ�	OTA_Authorization
*
*	�������ܣ�	����Authorization
*
*	��ڲ�����	ver��������汾�ţ����ڸ�ʽ��Ŀǰ��֧�ָ�ʽ"2018-10-31"
*				res����Ʒid
*				et������ʱ�䣬UTC��ֵ
*				access_key��������Կ
*				dev_name���豸��
*				authorization_buf������token��ָ��
*				authorization_buf_len������������(�ֽ�)
*
*	���ز�����	0-�ɹ�	����-ʧ��
*
*	˵����		��ǰ��֧��sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//����ǩ����Base64������ �� URL������
	char hmac_sha1_buf[64];							//����ǩ��
	char access_key_base64[64];						//����access_key��Base64������
	char string_for_signature[72];					//����string_for_signature������Ǽ��ܵ�key

//----------------------------------------------------�����Ϸ���--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------��access_key����Base64����----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//printf("access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------����string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	//printf("string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------����-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//printf("hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------�����ܽ������Base64����------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------��Base64����������URL����---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//printf("sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------����Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	//printf("Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	�������ƣ�	OneNET_RegisterDevice
//
//	�������ܣ�	�ڲ�Ʒ��ע��һ���豸
//
//	��ڲ�����	access_key��������Կ
//				pro_id����ƷID
//				serial��Ψһ�豸��
//				devid�����淵�ص�devid
//				key�����淵�ص�key
//
//	���ز�����	0-�ɹ�		1-ʧ��
//
//	˵����		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//���ܵ�key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		delay_ms(500);
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 240 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//�ȴ�ƽ̨��Ӧ
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			printf("create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	�������ƣ�	OneNet_DevLink
//
//	�������ܣ�	��onenet��������
//
//	��ڲ�����	��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		��onenetƽ̨��������
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//Э���

	unsigned char *dataPtr;
	
	char authorization_buf[160];
	
	_Bool status = 1;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n", "CONNECT"))
		delay_ms(500);
	
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
								authorization_buf, sizeof(authorization_buf), 0);
//	printf("OneNET_DevLink\r\n"
//							"NAME: %s,	PROID: %s,	KEY:%s\r\n"
//                        , DEVICE_NAME, PROID, authorization_buf);
	printf("\r\n");
	printf("===================\r\n");
	printf("���ڳ��Ե�¼OneNet!\r\n");
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ϴ�ƽ̨
		
		dataPtr = ESP8266_GetIPD(250);									//�ȴ�ƽ̨��Ӧ
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:printf("��¼�ɹ�!\r\n");status = 0;break;
					
					case 1:printf("��¼ʧ��: Э�����!\r\n");break;
					case 2:printf("��¼ʧ��: �Ƿ���clientid!\r\n");break;
					case 3:printf("��¼ʧ��: ������ʧ��!\r\n");break;
					case 4:printf("��¼ʧ��: �û������������!\r\n");break;
					case 5:printf("��¼ʧ��: �Ƿ�����(����token�Ƿ�)!\r\n");break;
					
					default:printf("��¼ʧ��: δ֪����!\r\n");break;
				}
			}
			else
			{
				printf("ƽ̨��������Ӧ!\r\n");
			}
		}
		else
		{
			printf("�ȴ���Ӧ��ʱ!\r\n");
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//ɾ��
	}
	else
		printf("MQTT Packet��� ʧ��!\r\n");
	
	printf("===================\r\n");
	printf("\r\n");
	return status;
	
}

//unsigned char OneNet_AR_FillBuf(char *buf)
//{
//	
//	char text[48];
//	
//	memset(text, 0, sizeof(text));
//	
//	strcpy(buf, "{\"id\":123,\"dp\":{");
//	
//	memset(text, 0, sizeof(text));
////	sprintf(text, "\"Tempreture\":[{\"v\":%f}],", sht20_info.tempreture);
//	strcat(buf, text);
//	
//	memset(text, 0, sizeof(text));
////	sprintf(text, "\"Humidity\":[{\"v\":%f}]", sht20_info.humidity);
//	strcat(buf, text);
//	
//	strcat(buf, "}}");
//	
//	return strlen(buf);

//}

////==========================================================
////	�������ƣ�	OneNet_SendData
////
////	�������ܣ�	�ϴ����ݵ�ƽ̨
////
////	��ڲ�����	type���������ݵĸ�ʽ
////
////	���ز�����	��
////
////	˵����		
////==========================================================
//void OneNet_SendData(void)
//{
//	
//	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//Э���
//	
//	char buf[256];
//	
//	short body_len = 0, i = 0;
//	
//	printf("Tips:	OneNet_SendData-MQTT\r\n");
//	
//	memset(buf, 0, sizeof(buf));
//	
//	body_len = OneNet_AR_FillBuf(buf);																	//��ȡ��ǰ��Ҫ���͵����������ܳ���
//	
//	if(body_len)
//	{
//		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
//		{
//			for(; i < body_len; i++)
//				mqttPacket._data[mqttPacket._len++] = buf[i];
//			
//			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
//			printf("Send %d Bytes\r\n", mqttPacket._len);
//			
//			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��
//		}
//		else
//			printf("WARN:	EDP_NewBuffer Failed\r\n");
//	}
//	
//}

//==========================================================
//	�������ƣ�	OneNET_Publish
//
//	�������ܣ�	������Ϣ
//
//	��ڲ�����	topic������������
//				msg����Ϣ����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���
	
	printf("Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������
		
		MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
	}

}

//==========================================================
//	�������ƣ�	OneNET_Subscribe
//
//	�������ܣ�	����
//
//	��ڲ�����	format������topic�ĸ�ʽ�����в�ƷID���豸���Ʋ����ø�ʽ��%s����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNET_Subscribe(char *format)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Э���
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), format, PROID, DEVICE_NAME);
	
	printf("���Զ��� Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//��ƽ̨���Ͷ�������
		
		MQTT_DeleteBuffer(&mqtt_packet);										//ɾ��
	}

}

void MQTT_Request_Response(cJSON* Request);
void MQTT_SET_Response(cJSON* SET);

//==========================================================
//	�������ƣ�	OneNet_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:																//���յ�Publish��Ϣ
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char topic[64];
				printf("\r\n");
				printf("===================\r\n");
				printf("�յ�Publish��Ϣ��\r\n");
				printf("-------------------\r\n");
				printf(" topic: %s\r\n topic_len: %d\r\n payload: %s\r\n payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);
				printf("-------------------\r\n");
				printf("===================\r\n");
				printf("\r\n");
				
				sprintf(topic, "$sys/%s/%s/thing/property/get", PROID, DEVICE_NAME);/* ����Ƿ���ƽ̨�������� */
				if(strcmp(cmdid_topic, topic) == 0)									
				{
					cJSON* Request_JSON = cJSON_Parse(req_payload);
					MQTT_Request_Response(Request_JSON);
					cJSON_Delete(Request_JSON);										/* �����ڴ� */
				}
				
				sprintf(topic, "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);/* ����Ƿ���ƽ̨�������� */
				if(strcmp(cmdid_topic, topic) == 0)									
				{
					cJSON* SET_JSON = cJSON_Parse(req_payload);
					MQTT_SET_Response(SET_JSON);
					cJSON_Delete(SET_JSON);											/* �����ڴ� */
				}
				
//				data_ptr = strstr(cmdid_topic, "request/");									//����cmdid
//				if(data_ptr)
//				{
//					char topic_buf[80], cmdid[40];
//					
//					data_ptr = strchr(data_ptr, '/');
//					data_ptr++;
//					
//					memcpy(cmdid, data_ptr, 36);											//����cmdid
//					cmdid[36] = 0;
//					
//					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/response/%s",
//															PROID, DEVICE_NAME, cmdid);
//					OneNET_Publish(topic_buf, "ojbk");										//�ظ�����
//				}
			}
		break;
		case MQTT_PKT_PUBACK:														//����Publish��Ϣ��ƽ̨�ظ���Ack
			printf("-------------------\r\n");
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				printf("MQTT Publish�� ���ͳɹ�!\r\n");
			printf("-------------------\r\n");
		break;
		
		case MQTT_PKT_SUBACK:																//����Subscribe��Ϣ��Ack
			printf("-------------------\r\n");
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				printf("MQTT ���ĳɹ�!\r\n");
			else
				printf("MQTT ����ʧ��!\r\n");
			printf("-------------------\r\n");
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//��ջ���
	
	if(result == -1)
		return;
	
//	dataPtr = strchr(req_payload, ':');					//����':'

//	if(dataPtr != NULL && result != -1)					//����ҵ���
//	{
//		dataPtr++;
//		
//		while(*dataPtr >= '0' && *dataPtr <= '9')		//�ж��Ƿ����·��������������
//		{
//			numBuf[num++] = *dataPtr++;
//		}
//		numBuf[num] = 0;
//		
//		num = atoi((const char *)numBuf);				//תΪ��ֵ��ʽ
//	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

/*===================================================MQTT���===================================================*/

//OneNetר��
//��ʱ�����߳�ʼ��
//���ܣ�
//1.������ά�ַ��������ӣ��ڽ��յ�ƽ̨���ݺ����������������������ﵽ�趨ֵ�ϱ�������Ϊ��������
//2.���£���ȡ���µ�DHT11���ݣ���ʱˢ�¡���������ǰһ�̲�һ�£����������ϱ�ƽ̨��
//3.DHT11æ�ȣ�dht11_read_data��ͷ�ȴ�������Ϊ0�����ü���������ֹ�޼����ȡDHT11����DHT11������
//4.TCPæ�ȣ�ESP8266_SendData����TCP͸������ͷ�ȴ�������Ϊ0�����ü���������ֹ�޼������TCPͨ�ŵ���TCP������
//Time��ʱ�ж�1ms
//t1������ʱ�䣬��λs
//t2�����ݸ���ʱ�䣬��λs
//t3��DHT11æ��ʱ�䣬��λms
//t4��TCPæ��ʱ�䣬��λms
void TIM_Tool_Init(int t1, int t2, int t3, int t4)
{
	Time1 = t1 * 1000;
	Time2 = t2 * 1000;
	Time3 = t3;
	Time4 = t4;
	btim_timx_int_init(720 - 1, 100 - 1);		/* ��ʱ����ʼ��,�ж�ʱ��1ms */
}

//OneNetר��
//����MQTT topic����
void MQTT_Common_Subscribe(void)
{
	char *dataPtr = NULL;
	
	printf("\r\n");
	printf("===================\r\n");
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/post/reply");	/* �豸�����ϱ���Ӧ */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* ƽ̨��Ӧ */
	dataPtr = NULL;
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/set");			/* �豸������������ */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* ƽ̨��Ӧ */
	dataPtr = NULL;
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/get");			/* �豸���Ի�ȡ���� */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* ƽ̨��Ӧ */
	dataPtr = NULL;
	
	printf("-------------------\r\n");
	printf("����MQTT topic�������!\r\n");
	printf("-------------------\r\n");
	printf("===================\r\n");
	printf("\r\n");
}

//OneNetָ���豸ר�ã���Ҫ�����豸���ԵĲ�ͬ�����ƣ�
//�豸�����ϱ�JSON���
//�ɹ�-JSON�����ȣ�ʧ��-0
//��ʽ��
//
//{
//	"id": "123",
//	"version": "1.0",
//	"params": {
//		"LED": {
//			"value": true
//		},
//		"humi": {
//			"value": 33
//		},
//		"temp": {
//			"value": 23
//		}
//	}
//}
uint8_t OneNet_AR_FillBuf(char** json_str)
{
	static int id = 1;											/* ��������ID */
	char string_buf[16];										/* �����ַ��������� */
	cJSON * json_buf = cJSON_CreateObject();					/* ����JSON������ */
	
	sprintf(string_buf, "%d", id);
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* ����ID��ֵ�� */
	id++;
	cJSON_AddStringToObject(json_buf, "version", "1.0");		/* ����汾�ż�ֵ�� */ //Ŀǰ��֧��1.0
	
	
    cJSON* params = cJSON_CreateObject();						/* ���� params ���� */ 
	
	// ��� LED ����
	cJSON* LED_obj = cJSON_CreateObject();
	if(LED == 0)
	{
		cJSON_AddFalseToObject(LED_obj, "value");
	}
	else
	{
		cJSON_AddTrueToObject(LED_obj, "value");
	}
	cJSON_AddItemToObject(params, "LED", LED_obj);				/* LED���� ���� params */

	// ��� humi ����
	cJSON* humi_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(humi_obj, "value", humi);
    cJSON_AddItemToObject(params, "humi", humi_obj);			/* humi���� ���� params */
	
	// ��� temp ����
	cJSON* temp_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(temp_obj, "value", temp);
    cJSON_AddItemToObject(params, "temp", temp_obj);			/* temp���� ���� params */
	
	
    cJSON_AddItemToObject(json_buf, "params", params);			/* �� params ���� �� */

    *json_str = cJSON_Print(json_buf);							/* ���� JSON �ַ��� */
	
    cJSON_Delete(json_buf);										/* �����ڴ� */
	
	return strlen(*json_str);
}

//OneNetר��
//�豸�����ϱ�
void MQTT_Attribute_Reporting(void)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* Э��� */
	short body_len = 0, i = 0;									/* ��־λ */
	char* Attribute_JSON = NULL;								/* JSON�ַ��������� */
	Attribute_JSON = (char *)malloc(sizeof(char) * 256);
	char *dataPtr = NULL;										/* ƽ̨��Ӧ������ */
	

	printf("\r\n");
	printf("===================\r\n");
	printf("�����ϱ�����\r\n");
	body_len = OneNet_AR_FillBuf(&Attribute_JSON);					/* ��ȡ����JSON�� */
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//���
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = Attribute_JSON[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
//			printf("Send %d Bytes\r\n", mqttPacket._len);
			printf("�����ϴ�����\r\n");
			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��
		}
		else
		{
			printf("����: �������ݰ���� ʧ��!\r\n");
		}
	}
	else
	{
		printf("����: ��ȡ�������ݰ� ʧ��!\r\n");
	}
	
	free(Attribute_JSON);											/* �ͷſռ� */
	
	dataPtr = ESP8266_GetIPD(100);			/* �ȴ�ƽ̨��Ӧ */
	if(dataPtr != NULL)
	{
		OneNet_RevPro(dataPtr);				/* ������Ӧ */
		printf("�豸�����ϱ� �ɹ�!\r\n");
		Interaction_Succeed();						/* �����ɹ� */
	}
	else
	{
		printf("�豸�����ϱ� �ϴ�ʧ��!\r\n");
		Interaction_Failure();						/* ����ʧ�� */
	}
	printf("===================\r\n");
	printf("\r\n");
}

//OneNetָ���豸ר�ã���Ҫ�����豸���ԵĲ�ͬ�����ƣ�
//ƽ̨������ӦJSON���
//�ɹ�-JSON�����ȣ�ʧ��-0
//��ʽ��
//
//{
//	"id": "123",
//	"code": 200,
//	"msg": "success",
//	"data": {
//		"LED": true,
//		"humi": 20,
//      "temp": 39
//   }
//}
uint8_t OneNet_RR_FillBuf(char** json_str, cJSON* Request)
{
	char string_buf[64];										/* �����ַ��������� */
	cJSON * json_buf = cJSON_CreateObject();					/* ����JSON������ */
	
	// ��ȡ���������е�ID�ֶ�
    cJSON *id_JSON = cJSON_GetObjectItem(Request, "id");
	strcpy(string_buf, id_JSON->valuestring);
	
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* ����ID��ֵ�� */

	cJSON_AddNumberToObject(json_buf, "code", 200);				/* ����״̬���ֵ�� */ //�ܵ��ⲽ��˵��һ���������ɹ�����200
	
	cJSON_AddStringToObject(json_buf, "msg", "success");		/* ���������Ϣ��ֵ�� */ //�ܵ��ⲽ��˵��һ���������ɹ���Ϣ��"success"
	
    cJSON* data = cJSON_CreateObject();							/* ���� data ���� */ 
	
	// ��ȡ���������е������ֶ�
    cJSON *fruits = cJSON_GetObjectItem(Request, "params");
	
	// ��������
    int array_size = cJSON_GetArraySize(fruits);
    for (int i = 0; i < array_size; i++) {
        cJSON *item = cJSON_GetArrayItem(fruits, i);
		
		if(strcmp(item->valuestring, "LED") == 0)
		{
			// ���� LED ����
			if(LED == 0)
			{
				cJSON_AddFalseToObject(data, "LED");
			}
			else
			{
				cJSON_AddTrueToObject(data, "LED");
			}
		}
		
		if(strcmp(item->valuestring, "humi") == 0)
		{
			// ���� humi ����
			cJSON_AddNumberToObject(data, "humi", humi);
		}
		
		if(strcmp(item->valuestring, "temp") == 0)
		{
			// ���� temp ����
			cJSON_AddNumberToObject(data, "temp", temp);
		}
    }
	
	
    cJSON_AddItemToObject(json_buf, "data", data);				/* �� params ��ӵ��� */

    *json_str = cJSON_Print(json_buf);							/* ���� JSON �ַ��� */

    cJSON_Delete(json_buf);										/* �����ڴ� */
	
	return strlen(*json_str);
}

//OneNetר��
//����������Ӧ
void MQTT_Request_Response(cJSON* Request)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* Э��� */
	short body_len = 0, i = 0;									/* ��־λ */
	char* Request_JSON = NULL;									/* JSON�ַ��������� */
	Request_JSON = (char *)malloc(sizeof(char) * 256);
	char topic[64];												/* topic�ַ��������� */

	printf("\r\n");
	printf("===================\r\n");
	printf("������Ӧ��������\r\n");
	body_len = OneNet_RR_FillBuf(&Request_JSON, Request);		/* ��ȡ����JSON�� */
	sprintf(topic, "$sys/%s/%s/thing/property/get_reply", PROID, DEVICE_NAME);/* ��ȡ��Ӧtopic */
	if(body_len)
	{
		if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, Request_JSON, strlen(Request_JSON), MQTT_QOS_LEVEL0, 0, 1, &mqttPacket) == 0)
		{
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//��ƽ̨���Ͷ�������
			printf("�����ϴ�����\r\n");
			MQTT_DeleteBuffer(&mqttPacket);										//ɾ��
			printf("-------------------\r\n");
			printf("��Ӧ��������ɹ�!\r\n");
			printf("-------------------\r\n");
			Interaction_Succeed();						/* �����ɹ� */
		}
		else
		{
			printf("����: �������ݰ���� ʧ��!\r\n");
			Interaction_Failure();						/* ����ʧ�� */
		}
	}
	else
	{
		printf("����: ��ȡ�������ݰ� ʧ��!\r\n");
		Interaction_Failure();						/* ����ʧ�� */
	}
	printf("===================\r\n");
	printf("\r\n");
	free(Request_JSON);												/* �ͷſռ� */
}

//OneNet��Ӧ����ר��
//ƽ̨������ӦJSON���
//�ɹ�-JSON�����ȣ�ʧ��-0
//��ʽ��
//
//{
//	"id": "123",
//	"code": 200,
//	"msg": "success"
//}
uint8_t OneNet_SR_FillBuf(char** json_str, cJSON* SET)
{
	char string_buf[64];										/* �����ַ��������� */
	cJSON * json_buf = cJSON_CreateObject();					/* ����JSON������ */
	
	// ��ȡ���������е�ID�ֶ�
    cJSON *id_JSON = cJSON_GetObjectItem(SET, "id");
	strcpy(string_buf, id_JSON->valuestring);
	
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* ����ID��ֵ�� */

	cJSON_AddNumberToObject(json_buf, "code", 200);				/* ����״̬���ֵ�� */ //�ܵ��ⲽ��˵��һ���������ɹ�����200
	
	cJSON_AddStringToObject(json_buf, "msg", "success");		/* ���������Ϣ��ֵ�� */ //�ܵ��ⲽ��˵��һ���������ɹ���Ϣ��"success"
	
    *json_str = cJSON_Print(json_buf);							/* ���� JSON �ַ��� */

    cJSON_Delete(json_buf);										/* �����ڴ� */
	
	return strlen(*json_str);
}

//OneNetָ���豸ר��
//����������Ӧִ��
//�������ø�ʽ��
//{
//  "id": "123",
//  "version": "1.0",
//  "params": {
//    "LED": true
//  }
//}
void MQTT_SET_Execute(cJSON* SET)
{
	// ��ȡ���������е�ID�ֶ�
    cJSON *params_JSON = cJSON_GetObjectItem(SET, "params");		/* ��ȡ params ���� */
	
	
	
	//��ȡ֧�ֱ����õ����ԣ���ǰ��֧��LED
	cJSON *LED_json = cJSON_GetObjectItem(params_JSON, "LED");		/* ��ȡ LED ���� */
	

	
	//ִ����������
	if(LED_json != NULL)
	{
		if((LED_json->type == cJSON_True && LED == 0) || (LED_json->type == cJSON_False && LED == 1))					
		{
			LED_Overturn();
		}
	}
	
}

//OneNetר��
//����������Ӧ
void MQTT_SET_Response(cJSON* SET)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* Э��� */
	short body_len = 0, i = 0;									/* ��־λ */
	char* SET_JSON = NULL;										/* JSON�ַ��������� */
	SET_JSON = (char *)malloc(sizeof(char) * 256);
	char topic[64];												/* topic�ַ��������� */

	printf("\r\n");
	printf("===================\r\n");
	printf("������Ӧ��������\r\n");
	
	MQTT_SET_Execute(SET);										/* ִ���������� */
	
	body_len = OneNet_SR_FillBuf(&SET_JSON, SET);				/* ��ȡ����JSON�� */

	sprintf(topic, "$sys/%s/%s/thing/property/set_reply", PROID, DEVICE_NAME);/* ��ȡ��Ӧtopic */
	if(body_len)
	{
		if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, SET_JSON, strlen(SET_JSON), MQTT_QOS_LEVEL0, 0, 1, &mqttPacket) == 0)
		{
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//��ƽ̨���Ͷ�������
			printf("�����ϴ�����\r\n");
			MQTT_DeleteBuffer(&mqttPacket);										//ɾ��
			printf("-------------------\r\n");
			printf("��Ӧ�������óɹ�!\r\n");
			printf("-------------------\r\n");
			Interaction_Succeed();						/* �����ɹ� */
		}
		else
		{
			printf("����: ��ȡ����������Ӧ��� ʧ��!\r\n");
			Interaction_Failure();						/* ����ʧ�� */
		}
	}
	else
	{
		printf("����: ��ȡ����������Ӧ�� ʧ��!\r\n");
		Interaction_Failure();						/* ����ʧ�� */
	}
	printf("===================\r\n");
	printf("\r\n");
	free(SET_JSON);												/* �ͷſռ� */
}

//�ù���ר��
//ƽ̨��������ģ��
//��ģ��������������һ�������ɹ���һ������ʧ��
//��Ҫ�����жϵ�ǰ�豸��ƽ̨�����Ƿ�������ÿ�ν���ʧ�ܶ����гͷ���ÿ�ν����ɹ�Ҳ�н���
//�����ν���ʧ�ܵ��½��ͷֹ��ͽ��ᱻ�豸ʶ��Ϊ������ͨ�ţ��豸�����������Խ������
//��ǰ���ƣ�
//���ͷ֣�0--100����ʼΪ100
//���ͷֵ���60�ֻ�������3�ν���ʧ�ܽ�����ͷ�����
//�ͷ������н����ɹ������н����֣�������ʧ����Ȼ�гͷ���
//����3�ν����ɹ����˳��ͷ����ҽ����ͷָֻ���60��
//���ͷ��ֽ���0���豸����
//������״̬�½����ɹ�����1�֣�����ʧ�ܳͷ�8��


uint8_t punishment = 0;			//�ͷ�״̬��־λ
uint8_t score = 100;			//���ͷ�
uint8_t succeed = 0; 			//�����ɹ�
uint8_t failure = 0; 			//����ʧ��

//�����ɹ�
void Interaction_Succeed(void)
{
	if(punishment)				//�ڳͷ�״̬
	{	
		/* ����״̬�ı� */
		succeed ++;				
		failure = 0;
		
		/*�������γɹ�*/
		if(succeed >= 3)		
		{
			punishment = 0;
			score = 60;
		}
	}
	else						//������״̬
	{
		/* ����״̬�ı� */
		succeed ++;				
		failure = 0;
		
		/* �ӷ� */
		if(score < 100)			
		{
			score ++;
		}
	}
}

//����ʧ��
void Interaction_Failure(void)
{
	if(punishment)				//�ڳͷ�״̬
	{	
		/* ����״̬�ı� */		
		succeed = 0;
		failure ++;
		
		/* ���� */
		if(score > 8)			
		{
			score -= 8;
		}
		else
		{
			printf("---------------------\r\n");
			printf("����: ��ǰͨ����������!\r\n");
			printf("���ڳ��������豸!\r\n");
			printf("---------------------\r\n");
			HAL_NVIC_SystemReset();	//�ּ���0�������豸
		}
	}
	else						//������״̬
	{
		/* ����״̬�ı� */		
		succeed = 0;
		failure ++;
		
		/* ���� */
		if(score > 8)			
		{
			score -= 8;
		}
		else
		{
			printf("---------------------\r\n");
			printf("����: ��ǰͨ����������!\r\n");
			printf("���ڳ��������豸!\r\n");
			printf("---------------------\r\n");
			HAL_NVIC_SystemReset();	//�ּ���0�������豸
		}
		
		/*��������ʧ�ܻ����60��*/
		if(failure >= 3 ||score < 60)		
		{
			punishment = 1;
			printf("---------------------\r\n");
			printf("����: ��ǰͨ�������ϲ�!\r\n");
			printf("---------------------\r\n");
		}
		
	}
}

/*===================================================HTTP���===================================================*/

//��ȡHTTP��Ӧ����
char* HTTP_Principal(const char* s) {
    // ���ҵ�һ��������
    const char* start = strchr(s, '{');
    if (start == NULL) {
        return NULL;
    }
    
    // ��ʼ���������ͱ���λ��
    int count = 1;
    const char* ptr = start + 1;
    
    // �����ַ���������Ƕ������
    while (*ptr != '\0' && count > 0) {
        if (*ptr == '{') {
            count++;
        } else if (*ptr == '}') {
            count--;
        }
        
        // �ҵ�ƥ���������ʱ����ѭ��
        if (count == 0) {
            break;
        }
        ptr++;
    }
    
    // ���δ�ҵ�ƥ��������ţ����� NULL
    if (count != 0) {
        return NULL;
    }
    
    // �����Ӵ����Ȳ������ڴ�
    size_t length = ptr - start + 1; // �����������һ�����
    char* result = (char*)malloc(length + 1); // +1 ���ڴ�� '\0'
    if (result == NULL) {
        return NULL; // �ڴ����ʧ��
    }
    
    // �����Ӵ��������ֹ��
    memcpy(result, start, length);
    result[length] = '\0';
    return result;
}

//OneNet��ȡƽ̨������APIר��
//��ȡ��ȡƽ̨����API��HTTP��Ӧ�����е�����
//HTTP��Ӧ������response��ָ��identifier�󣬷��ظ�ʽΪ{"data_type":"value"}������data_type��value�����ַ�������
char* OneNet_Data_Analysis(const char *response, const char *identifier) {
    cJSON *root = cJSON_Parse(response);
    if (!root) return NULL;

    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!data || data->type != cJSON_Array) {
        cJSON_Delete(root);
        return NULL;
    }

    int array_size = cJSON_GetArraySize(data);
    for (int i = 0; i < array_size; i++) {
        cJSON *target = cJSON_GetArrayItem(data, i);
        if (!target) continue;

        // ��ȡ identifier �ֶ�
        cJSON *id_item = cJSON_GetObjectItem(target, "identifier");
        if (!id_item || id_item->type != cJSON_String || 
            strcmp(id_item->valuestring, identifier) != 0) {
            continue;
        }

        // ��ȡ data_type �� value
        cJSON *data_type_item = cJSON_GetObjectItem(target, "data_type");
        cJSON *value_item = cJSON_GetObjectItem(target, "value");
        if (!data_type_item || data_type_item->type != cJSON_String ||
            !value_item || value_item->type != cJSON_String) {
            continue;
        }

        // ���� {data_type: "value"} �ַ���
        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, data_type_item->valuestring, value_item->valuestring);
        char *output = cJSON_PrintUnformatted(result);

        // �����ڴ�
        cJSON_Delete(result);
        cJSON_Delete(root);
        return output;
    }

    cJSON_Delete(root);
    return NULL;
}

//OneNet API�ӿ�ר��
//GET�������
char* GET_Burstification(const char *URL, const char *query, const char *authorization)
{
	char package[512];
	memset(package, 0, sizeof(package));
	
	//��������
	strcat(package, "GET ");
	//URL
	strcat(package, URL);
	//?
	strcat(package, "?");
	//����
	strcat(package, query);
	//HTTP�����ʽ
	strcat(package, " HTTP/1.1\r\nHost: iot-api.heclouds.com\r\nAccept: application/json, text/plain, */* \r\nauthorization:");
	//��ȫ��Ȩ��Կ
	strcat(package, authorization);
	//��β
	strcat(package, "\r\n\r\n");
	
	return package;
}

//OneNetָ���豸ר��
//Ӳ��״̬ͬ����ʼ��
void OneNet_Unify(void)
{
	char query[128];
	char authorization[256];
	char package[512];
	int len;
	char *dataPtr = NULL;
	char text[32];
	
	printf("\r\n");
	printf("===================\r\n");
	
	//��OneNet API�ӿڷ���������TCP����
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"iot-api.heclouds.com\",80\r\n", "CONNECT"))
		delay_ms(500);
	
	//���л�ȡ��������API���
	sprintf(query, "product_id=%s&device_name=%s", PROID, DEVICE_NAME);
	OneNET_Authorization("2018-10-31", PROID, 1768366435, ACCESS_KEY, DEVICE_NAME,
							authorization, sizeof(authorization), 0);
	strcpy(package, GET_Burstification("/thingmodel/query-device-property", query, authorization));
	
	//���Ͱ�
	len = strlen(package);
	ESP8266_SendData(package, len);
	
	//�ȴ�����
	while(dataPtr == NULL)dataPtr = ESP8266_GetIPD(100);
	
	//��ȡӲ������
	char *t = HTTP_Principal(dataPtr);
	strcpy(text, OneNet_Data_Analysis(t, "LED"));
	free(t);
	
	//ͬ��Ӳ��״̬������
	if(text != NULL)
	{
		if(strstr(text, "true") != NULL)			//�ҵ�Ϊ��
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
			LED = 1;
		}
		else										//����Ϊ��
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
			LED = 0;
		}
		printf("OneNet ����ͬ���ɹ�!\r\n");
	}
	else
	{
		printf("OneNet ����ͬ��ʧ��!\r\n");
	}
	printf("===================\r\n");
	printf("\r\n");
	
	
}

