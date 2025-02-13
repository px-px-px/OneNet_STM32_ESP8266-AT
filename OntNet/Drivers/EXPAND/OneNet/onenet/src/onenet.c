/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f1xx.h"

//网络设备
#include "esp8266.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//算法
#include "base64.h"
#include "hmac_sha1.h"

//硬件驱动
#include "usart.h"
#include "delay.h"
#include "./BSP/LED/LED.h"
#include "./BSP/DHT11/dht11.h"
#include "./BSP/TIMER/btim.h"

//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"


#define PROID			"{产品ID}"

#define ACCESS_KEY		"{产品密钥}"

#define DEVICE_NAME		"{设备名称}"


char devid[16];

char key[48];


extern unsigned char esp8266_buf[512];

void Interaction_Succeed(void);
void Interaction_Failure(void);


/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
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
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[64];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key

//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//printf("access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	//printf("string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//printf("hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//printf("sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	//printf("Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品中注册一个设备
//
//	入口参数：	access_key：访问密钥
//				pro_id：产品ID
//				serial：唯一设备号
//				devid：保存返回的devid
//				key：保存返回的key
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//加密的key
	
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
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
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
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	0-成功	1-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

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
	printf("正在尝试登录OneNet!\r\n");
	
	if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:printf("登录成功!\r\n");status = 0;break;
					
					case 1:printf("登录失败: 协议错误!\r\n");break;
					case 2:printf("登录失败: 非法的clientid!\r\n");break;
					case 3:printf("登录失败: 服务器失败!\r\n");break;
					case 4:printf("登录失败: 用户名或密码错误!\r\n");break;
					case 5:printf("登录失败: 非法链接(比如token非法)!\r\n");break;
					
					default:printf("登录失败: 未知错误!\r\n");break;
				}
			}
			else
			{
				printf("平台非正常响应!\r\n");
			}
		}
		else
		{
			printf("等待响应超时!\r\n");
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
		printf("MQTT Packet组包 失败!\r\n");
	
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
////	函数名称：	OneNet_SendData
////
////	函数功能：	上传数据到平台
////
////	入口参数：	type：发送数据的格式
////
////	返回参数：	无
////
////	说明：		
////==========================================================
//void OneNet_SendData(void)
//{
//	
//	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
//	
//	char buf[256];
//	
//	short body_len = 0, i = 0;
//	
//	printf("Tips:	OneNet_SendData-MQTT\r\n");
//	
//	memset(buf, 0, sizeof(buf));
//	
//	body_len = OneNet_AR_FillBuf(buf);																	//获取当前需要发送的数据流的总长度
//	
//	if(body_len)
//	{
//		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
//		{
//			for(; i < body_len; i++)
//				mqttPacket._data[mqttPacket._len++] = buf[i];
//			
//			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
//			printf("Send %d Bytes\r\n", mqttPacket._len);
//			
//			MQTT_DeleteBuffer(&mqttPacket);															//删包
//		}
//		else
//			printf("WARN:	EDP_NewBuffer Failed\r\n");
//	}
//	
//}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	printf("Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	format：订阅topic的格式，其中产品ID和设备名称部分用格式符%s代替
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(char *format)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包
	
	char topic_buf[56];
	const char *topic = topic_buf;
	
	snprintf(topic_buf, sizeof(topic_buf), format, PROID, DEVICE_NAME);
	
	printf("尝试订阅 Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}

}

void MQTT_Request_Response(cJSON* Request);
void MQTT_SET_Response(cJSON* SET);

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
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
		case MQTT_PKT_PUBLISH:																//接收的Publish消息
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char topic[64];
				printf("\r\n");
				printf("===================\r\n");
				printf("收到Publish消息：\r\n");
				printf("-------------------\r\n");
				printf(" topic: %s\r\n topic_len: %d\r\n payload: %s\r\n payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);
				printf("-------------------\r\n");
				printf("===================\r\n");
				printf("\r\n");
				
				sprintf(topic, "$sys/%s/%s/thing/property/get", PROID, DEVICE_NAME);/* 检测是否是平台请求属性 */
				if(strcmp(cmdid_topic, topic) == 0)									
				{
					cJSON* Request_JSON = cJSON_Parse(req_payload);
					MQTT_Request_Response(Request_JSON);
					cJSON_Delete(Request_JSON);										/* 清理内存 */
				}
				
				sprintf(topic, "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);/* 检测是否是平台请求属性 */
				if(strcmp(cmdid_topic, topic) == 0)									
				{
					cJSON* SET_JSON = cJSON_Parse(req_payload);
					MQTT_SET_Response(SET_JSON);
					cJSON_Delete(SET_JSON);											/* 清理内存 */
				}
				
//				data_ptr = strstr(cmdid_topic, "request/");									//查找cmdid
//				if(data_ptr)
//				{
//					char topic_buf[80], cmdid[40];
//					
//					data_ptr = strchr(data_ptr, '/');
//					data_ptr++;
//					
//					memcpy(cmdid, data_ptr, 36);											//复制cmdid
//					cmdid[36] = 0;
//					
//					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/cmd/response/%s",
//															PROID, DEVICE_NAME, cmdid);
//					OneNET_Publish(topic_buf, "ojbk");										//回复命令
//				}
			}
		break;
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
			printf("-------------------\r\n");
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				printf("MQTT Publish包 发送成功!\r\n");
			printf("-------------------\r\n");
		break;
		
		case MQTT_PKT_SUBACK:																//发送Subscribe消息的Ack
			printf("-------------------\r\n");
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				printf("MQTT 订阅成功!\r\n");
			else
				printf("MQTT 订阅失败!\r\n");
			printf("-------------------\r\n");
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
	
//	dataPtr = strchr(req_payload, ':');					//搜索':'

//	if(dataPtr != NULL && result != -1)					//如果找到了
//	{
//		dataPtr++;
//		
//		while(*dataPtr >= '0' && *dataPtr <= '9')		//判断是否是下发的命令控制数据
//		{
//			numBuf[num++] = *dataPtr++;
//		}
//		numBuf[num] = 0;
//		
//		num = atoi((const char *)numBuf);				//转为数值形式
//	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}

/*===================================================MQTT相关===================================================*/

//OneNet专用
//定时器工具初始化
//功能：
//1.心跳（维持服务器连接，在接收到平台数据后清除计数器，如果计数器达到设定值上报属性作为心跳包）
//2.更新（获取最新的DHT11数据，定时刷新。若数据与前一刻不一致，则将新数据上报平台）
//3.DHT11忙等（dht11_read_data开头等待计数器为0并重置计数器。防止无间隔读取DHT11导致DHT11断连）
//4.TCP忙等（ESP8266_SendData进行TCP透传，开头等待计数器为0并重置计数器。防止无间隔进行TCP通信导致TCP断连）
//Time定时中断1ms
//t1：心跳时间，单位s
//t2：数据更新时间，单位s
//t3：DHT11忙等时间，单位ms
//t4：TCP忙等时间，单位ms
void TIM_Tool_Init(int t1, int t2, int t3, int t4)
{
	Time1 = t1 * 1000;
	Time2 = t2 * 1000;
	Time3 = t3;
	Time4 = t4;
	btim_timx_int_init(720 - 1, 100 - 1);		/* 定时器初始化,中断时间1ms */
}

//OneNet专用
//常用MQTT topic订阅
void MQTT_Common_Subscribe(void)
{
	char *dataPtr = NULL;
	
	printf("\r\n");
	printf("===================\r\n");
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/post/reply");	/* 设备属性上报响应 */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* 平台响应 */
	dataPtr = NULL;
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/set");			/* 设备属性设置请求 */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* 平台响应 */
	dataPtr = NULL;
	
	do
	{
		OneNET_Subscribe("$sys/%s/%s/thing/property/get");			/* 设备属性获取请求 */
		dataPtr = ESP8266_GetIPD(100);
	} while(dataPtr == NULL);
	OneNet_RevPro(dataPtr);											/* 平台响应 */
	dataPtr = NULL;
	
	printf("-------------------\r\n");
	printf("常用MQTT topic订阅完成!\r\n");
	printf("-------------------\r\n");
	printf("===================\r\n");
	printf("\r\n");
}

//OneNet指定设备专用（需要根据设备属性的不同而定制）
//设备属性上报JSON打包
//成功-JSON包长度，失败-0
//格式：
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
	static int id = 1;											/* 创建自增ID */
	char string_buf[16];										/* 创建字符串缓冲区 */
	cJSON * json_buf = cJSON_CreateObject();					/* 创建JSON根对象 */
	
	sprintf(string_buf, "%d", id);
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* 插入ID键值对 */
	id++;
	cJSON_AddStringToObject(json_buf, "version", "1.0");		/* 插入版本号键值对 */ //目前仅支持1.0
	
	
    cJSON* params = cJSON_CreateObject();						/* 创建 params 对象 */ 
	
	// 添加 LED 数据
	cJSON* LED_obj = cJSON_CreateObject();
	if(LED == 0)
	{
		cJSON_AddFalseToObject(LED_obj, "value");
	}
	else
	{
		cJSON_AddTrueToObject(LED_obj, "value");
	}
	cJSON_AddItemToObject(params, "LED", LED_obj);				/* LED数据 插入 params */

	// 添加 humi 数据
	cJSON* humi_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(humi_obj, "value", humi);
    cJSON_AddItemToObject(params, "humi", humi_obj);			/* humi数据 插入 params */
	
	// 添加 temp 数据
	cJSON* temp_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(temp_obj, "value", temp);
    cJSON_AddItemToObject(params, "temp", temp_obj);			/* temp数据 插入 params */
	
	
    cJSON_AddItemToObject(json_buf, "params", params);			/* 将 params 插入 根 */

    *json_str = cJSON_Print(json_buf);							/* 生成 JSON 字符串 */
	
    cJSON_Delete(json_buf);										/* 清理内存 */
	
	return strlen(*json_str);
}

//OneNet专用
//设备属性上报
void MQTT_Attribute_Reporting(void)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* 协议包 */
	short body_len = 0, i = 0;									/* 标志位 */
	char* Attribute_JSON = NULL;								/* JSON字符串缓冲区 */
	Attribute_JSON = (char *)malloc(sizeof(char) * 256);
	char *dataPtr = NULL;										/* 平台响应缓冲区 */
	

	printf("\r\n");
	printf("===================\r\n");
	printf("尝试上报属性\r\n");
	body_len = OneNet_AR_FillBuf(&Attribute_JSON);					/* 获取属性JSON包 */
	if(body_len)
	{
		if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
		{
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = Attribute_JSON[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据到平台
//			printf("Send %d Bytes\r\n", mqttPacket._len);
			printf("正在上传数据\r\n");
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
		else
		{
			printf("错误: 最新数据包组包 失败!\r\n");
		}
	}
	else
	{
		printf("错误: 获取最新数据包 失败!\r\n");
	}
	
	free(Attribute_JSON);											/* 释放空间 */
	
	dataPtr = ESP8266_GetIPD(100);			/* 等待平台响应 */
	if(dataPtr != NULL)
	{
		OneNet_RevPro(dataPtr);				/* 解析响应 */
		printf("设备属性上报 成功!\r\n");
		Interaction_Succeed();						/* 交互成功 */
	}
	else
	{
		printf("设备属性上报 上传失败!\r\n");
		Interaction_Failure();						/* 交互失败 */
	}
	printf("===================\r\n");
	printf("\r\n");
}

//OneNet指定设备专用（需要根据设备属性的不同而定制）
//平台请求响应JSON打包
//成功-JSON包长度，失败-0
//格式：
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
	char string_buf[64];										/* 创建字符串缓冲区 */
	cJSON * json_buf = cJSON_CreateObject();					/* 创建JSON根对象 */
	
	// 提取请求主体中的ID字段
    cJSON *id_JSON = cJSON_GetObjectItem(Request, "id");
	strcpy(string_buf, id_JSON->valuestring);
	
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* 插入ID键值对 */

	cJSON_AddNumberToObject(json_buf, "code", 200);				/* 插入状态码键值对 */ //能到这步就说明一切正常，成功码是200
	
	cJSON_AddStringToObject(json_buf, "msg", "success");		/* 插入错误消息键值对 */ //能到这步就说明一切正常，成功消息给"success"
	
    cJSON* data = cJSON_CreateObject();							/* 创建 data 对象 */ 
	
	// 提取请求主体中的数组字段
    cJSON *fruits = cJSON_GetObjectItem(Request, "params");
	
	// 遍历数组
    int array_size = cJSON_GetArraySize(fruits);
    for (int i = 0; i < array_size; i++) {
        cJSON *item = cJSON_GetArrayItem(fruits, i);
		
		if(strcmp(item->valuestring, "LED") == 0)
		{
			// 插入 LED 数据
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
			// 插入 humi 数据
			cJSON_AddNumberToObject(data, "humi", humi);
		}
		
		if(strcmp(item->valuestring, "temp") == 0)
		{
			// 插入 temp 数据
			cJSON_AddNumberToObject(data, "temp", temp);
		}
    }
	
	
    cJSON_AddItemToObject(json_buf, "data", data);				/* 将 params 添加到根 */

    *json_str = cJSON_Print(json_buf);							/* 生成 JSON 字符串 */

    cJSON_Delete(json_buf);										/* 清理内存 */
	
	return strlen(*json_str);
}

//OneNet专用
//属性请求响应
void MQTT_Request_Response(cJSON* Request)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* 协议包 */
	short body_len = 0, i = 0;									/* 标志位 */
	char* Request_JSON = NULL;									/* JSON字符串缓冲区 */
	Request_JSON = (char *)malloc(sizeof(char) * 256);
	char topic[64];												/* topic字符串缓冲区 */

	printf("\r\n");
	printf("===================\r\n");
	printf("尝试响应属性请求\r\n");
	body_len = OneNet_RR_FillBuf(&Request_JSON, Request);		/* 获取属性JSON包 */
	sprintf(topic, "$sys/%s/%s/thing/property/get_reply", PROID, DEVICE_NAME);/* 获取响应topic */
	if(body_len)
	{
		if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, Request_JSON, strlen(Request_JSON), MQTT_QOS_LEVEL0, 0, 1, &mqttPacket) == 0)
		{
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//向平台发送订阅请求
			printf("正在上传数据\r\n");
			MQTT_DeleteBuffer(&mqttPacket);										//删包
			printf("-------------------\r\n");
			printf("响应属性请求成功!\r\n");
			printf("-------------------\r\n");
			Interaction_Succeed();						/* 交互成功 */
		}
		else
		{
			printf("错误: 最新数据包组包 失败!\r\n");
			Interaction_Failure();						/* 交互失败 */
		}
	}
	else
	{
		printf("错误: 获取最新数据包 失败!\r\n");
		Interaction_Failure();						/* 交互失败 */
	}
	printf("===================\r\n");
	printf("\r\n");
	free(Request_JSON);												/* 释放空间 */
}

//OneNet回应设置专用
//平台设置响应JSON打包
//成功-JSON包长度，失败-0
//格式：
//
//{
//	"id": "123",
//	"code": 200,
//	"msg": "success"
//}
uint8_t OneNet_SR_FillBuf(char** json_str, cJSON* SET)
{
	char string_buf[64];										/* 创建字符串缓冲区 */
	cJSON * json_buf = cJSON_CreateObject();					/* 创建JSON根对象 */
	
	// 提取请求主体中的ID字段
    cJSON *id_JSON = cJSON_GetObjectItem(SET, "id");
	strcpy(string_buf, id_JSON->valuestring);
	
	cJSON_AddStringToObject(json_buf, "id", string_buf);		/* 插入ID键值对 */

	cJSON_AddNumberToObject(json_buf, "code", 200);				/* 插入状态码键值对 */ //能到这步就说明一切正常，成功码是200
	
	cJSON_AddStringToObject(json_buf, "msg", "success");		/* 插入错误消息键值对 */ //能到这步就说明一切正常，成功消息给"success"
	
    *json_str = cJSON_Print(json_buf);							/* 生成 JSON 字符串 */

    cJSON_Delete(json_buf);										/* 清理内存 */
	
	return strlen(*json_str);
}

//OneNet指定设备专用
//属性设置响应执行
//属性设置格式：
//{
//  "id": "123",
//  "version": "1.0",
//  "params": {
//    "LED": true
//  }
//}
void MQTT_SET_Execute(cJSON* SET)
{
	// 提取请求主体中的ID字段
    cJSON *params_JSON = cJSON_GetObjectItem(SET, "params");		/* 提取 params 数据 */
	
	
	
	//提取支持被设置的属性，当前仅支持LED
	cJSON *LED_json = cJSON_GetObjectItem(params_JSON, "LED");		/* 提取 LED 数据 */
	

	
	//执行属性设置
	if(LED_json != NULL)
	{
		if((LED_json->type == cJSON_True && LED == 0) || (LED_json->type == cJSON_False && LED == 1))					
		{
			LED_Overturn();
		}
	}
	
}

//OneNet专用
//属性设置响应
void MQTT_SET_Response(cJSON* SET)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};			/* 协议包 */
	short body_len = 0, i = 0;									/* 标志位 */
	char* SET_JSON = NULL;										/* JSON字符串缓冲区 */
	SET_JSON = (char *)malloc(sizeof(char) * 256);
	char topic[64];												/* topic字符串缓冲区 */

	printf("\r\n");
	printf("===================\r\n");
	printf("尝试响应属性设置\r\n");
	
	MQTT_SET_Execute(SET);										/* 执行属性设置 */
	
	body_len = OneNet_SR_FillBuf(&SET_JSON, SET);				/* 获取属性JSON包 */

	sprintf(topic, "$sys/%s/%s/thing/property/set_reply", PROID, DEVICE_NAME);/* 获取响应topic */
	if(body_len)
	{
		if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, SET_JSON, strlen(SET_JSON), MQTT_QOS_LEVEL0, 0, 1, &mqttPacket) == 0)
		{
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//向平台发送订阅请求
			printf("正在上传数据\r\n");
			MQTT_DeleteBuffer(&mqttPacket);										//删包
			printf("-------------------\r\n");
			printf("响应属性设置成功!\r\n");
			printf("-------------------\r\n");
			Interaction_Succeed();						/* 交互成功 */
		}
		else
		{
			printf("错误: 获取属性设置响应组包 失败!\r\n");
			Interaction_Failure();						/* 交互失败 */
		}
	}
	else
	{
		printf("错误: 获取属性设置响应包 失败!\r\n");
		Interaction_Failure();						/* 交互失败 */
	}
	printf("===================\r\n");
	printf("\r\n");
	free(SET_JSON);												/* 释放空间 */
}

//该工程专用
//平台交互奖惩模块
//该模块有两个函数，一个交互成功，一个交互失败
//主要用于判断当前设备与平台交互是否正常，每次交互失败都会有惩罚，每次交互成功也有奖励
//如果多次交互失败导致奖惩分过低将会被设备识别为不正常通信，设备将尝试重启以解决问题
//当前机制：
//奖惩分：0--100，初始为100
//奖惩分低于60分或者连续3次交互失败将进入惩罚机制
//惩罚机制中交互成功不再有奖励分，但交互失败仍然有惩罚分
//连续3次交互成功将退出惩罚并且将奖惩分恢复至60分
//当惩罚分降到0后设备重启
//在正常状态下交互成功奖励1分，交互失败惩罚8分


uint8_t punishment = 0;			//惩罚状态标志位
uint8_t score = 100;			//奖惩分
uint8_t succeed = 0; 			//连续成功
uint8_t failure = 0; 			//连续失败

//交互成功
void Interaction_Succeed(void)
{
	if(punishment)				//在惩罚状态
	{	
		/* 连续状态改变 */
		succeed ++;				
		failure = 0;
		
		/*连续三次成功*/
		if(succeed >= 3)		
		{
			punishment = 0;
			score = 60;
		}
	}
	else						//在正常状态
	{
		/* 连续状态改变 */
		succeed ++;				
		failure = 0;
		
		/* 加分 */
		if(score < 100)			
		{
			score ++;
		}
	}
}

//交互失败
void Interaction_Failure(void)
{
	if(punishment)				//在惩罚状态
	{	
		/* 连续状态改变 */		
		succeed = 0;
		failure ++;
		
		/* 减分 */
		if(score > 8)			
		{
			score -= 8;
		}
		else
		{
			printf("---------------------\r\n");
			printf("错误: 当前通信质量极差!\r\n");
			printf("正在尝试重启设备!\r\n");
			printf("---------------------\r\n");
			HAL_NVIC_SystemReset();	//分减到0，重启设备
		}
	}
	else						//在正常状态
	{
		/* 连续状态改变 */		
		succeed = 0;
		failure ++;
		
		/* 减分 */
		if(score > 8)			
		{
			score -= 8;
		}
		else
		{
			printf("---------------------\r\n");
			printf("错误: 当前通信质量极差!\r\n");
			printf("正在尝试重启设备!\r\n");
			printf("---------------------\r\n");
			HAL_NVIC_SystemReset();	//分减到0，重启设备
		}
		
		/*连续三次失败或低于60分*/
		if(failure >= 3 ||score < 60)		
		{
			punishment = 1;
			printf("---------------------\r\n");
			printf("警告: 当前通信质量较差!\r\n");
			printf("---------------------\r\n");
		}
		
	}
}

/*===================================================HTTP相关===================================================*/

//提取HTTP响应主体
char* HTTP_Principal(const char* s) {
    // 查找第一个左花括号
    const char* start = strchr(s, '{');
    if (start == NULL) {
        return NULL;
    }
    
    // 初始化计数器和遍历位置
    int count = 1;
    const char* ptr = start + 1;
    
    // 遍历字符串，处理嵌套括号
    while (*ptr != '\0' && count > 0) {
        if (*ptr == '{') {
            count++;
        } else if (*ptr == '}') {
            count--;
        }
        
        // 找到匹配的右括号时跳出循环
        if (count == 0) {
            break;
        }
        ptr++;
    }
    
    // 如果未找到匹配的右括号，返回 NULL
    if (count != 0) {
        return NULL;
    }
    
    // 计算子串长度并分配内存
    size_t length = ptr - start + 1; // 包含结束的右花括号
    char* result = (char*)malloc(length + 1); // +1 用于存放 '\0'
    if (result == NULL) {
        return NULL; // 内存分配失败
    }
    
    // 复制子串并添加终止符
    memcpy(result, start, length);
    result[length] = '\0';
    return result;
}

//OneNet获取平台数据类API专用
//提取获取平台数据API的HTTP响应主体中的数据
//HTTP响应主体是response，指定identifier后，返回格式为{"data_type":"value"}，其中data_type和value都是字符串类型
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

        // 获取 identifier 字段
        cJSON *id_item = cJSON_GetObjectItem(target, "identifier");
        if (!id_item || id_item->type != cJSON_String || 
            strcmp(id_item->valuestring, identifier) != 0) {
            continue;
        }

        // 提取 data_type 和 value
        cJSON *data_type_item = cJSON_GetObjectItem(target, "data_type");
        cJSON *value_item = cJSON_GetObjectItem(target, "value");
        if (!data_type_item || data_type_item->type != cJSON_String ||
            !value_item || value_item->type != cJSON_String) {
            continue;
        }

        // 构造 {data_type: "value"} 字符串
        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, data_type_item->valuestring, value_item->valuestring);
        char *output = cJSON_PrintUnformatted(result);

        // 清理内存
        cJSON_Delete(result);
        cJSON_Delete(root);
        return output;
    }

    cJSON_Delete(root);
    return NULL;
}

//OneNet API接口专用
//GET请求组包
char* GET_Burstification(const char *URL, const char *query, const char *authorization)
{
	char package[512];
	memset(package, 0, sizeof(package));
	
	//请求类型
	strcat(package, "GET ");
	//URL
	strcat(package, URL);
	//?
	strcat(package, "?");
	//参数
	strcat(package, query);
	//HTTP请求格式
	strcat(package, " HTTP/1.1\r\nHost: iot-api.heclouds.com\r\nAccept: application/json, text/plain, */* \r\nauthorization:");
	//安全鉴权密钥
	strcat(package, authorization);
	//结尾
	strcat(package, "\r\n\r\n");
	
	return package;
}

//OneNet指定设备专用
//硬件状态同步初始化
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
	
	//与OneNet API接口服务器建立TCP连接
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"iot-api.heclouds.com\",80\r\n", "CONNECT"))
		delay_ms(500);
	
	//进行获取最新数据API组包
	sprintf(query, "product_id=%s&device_name=%s", PROID, DEVICE_NAME);
	OneNET_Authorization("2018-10-31", PROID, 1768366435, ACCESS_KEY, DEVICE_NAME,
							authorization, sizeof(authorization), 0);
	strcpy(package, GET_Burstification("/thingmodel/query-device-property", query, authorization));
	
	//发送包
	len = strlen(package);
	ESP8266_SendData(package, len);
	
	//等待接收
	while(dataPtr == NULL)dataPtr = ESP8266_GetIPD(100);
	
	//提取硬件数据
	char *t = HTTP_Principal(dataPtr);
	strcpy(text, OneNet_Data_Analysis(t, "LED"));
	free(t);
	
	//同步硬件状态和数据
	if(text != NULL)
	{
		if(strstr(text, "true") != NULL)			//找到为真
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
			LED = 1;
		}
		else										//否则为假
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
			LED = 0;
		}
		printf("OneNet 数据同步成功!\r\n");
	}
	else
	{
		printf("OneNet 数据同步失败!\r\n");
	}
	printf("===================\r\n");
	printf("\r\n");
	
	
}

