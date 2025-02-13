#ifndef _ONENET_H_
#define _ONENET_H_





_Bool OneNET_RegisterDevice(void);

_Bool OneNet_DevLink(void);

void OneNet_SendData(void);

void OneNET_Subscribe(char *format);

void OneNet_RevPro(unsigned char *cmd);



/*²¹³ä*/
void TIM_Tool_Init(int t1, int t2, int t3, int t4);
void Interaction_Succeed(void);
void Interaction_Failure(void);
void MQTT_Common_Subscribe(void);
void MQTT_Attribute_Reporting(void);
void OneNet_Unify(void);


#endif
