# STM32F1与OneNet数据交互
## 交互类型：
   * 设备通过API接口获取OneNet平台的数据
   * 设备向OneNet平台上报属性(MQTT接入)
   * OneNet平台下发属性设置(MQTT接入)
   * OneNet平台下发属性请求(MQTT接入)
## 调试信息：
![public/image/sampling.png](https://github.com/px-px-px/OneNet_STM32_ESP8266-AT/blob/master/sampling.png)
   * 通过USART1将调试信息打印到串口助手
## 示例条件：
   * OneNet平台（数据：LED bool wr, humi int32 r , temp int32 r）
   * ESP8266（有支持TCP透传的固件就行）
   * STM32F103ZET6（正点原子 战舰）
   * DHT11
## 交互情境：
   * 设备硬件初始化时，设备通过API接口向平台请求数据，并根据平台数据进行初始化（如：LED初始状态），从而达到平台和设备的数据一致
   * 当设备中数据改变时（如：温度、湿度、按键或平台使LED状态改变后），设备向OneNet上报属性
   * 当接收到平台下发的属性设置后，解析其中允许被修改的属性，并修改设备中的数据、改变硬件状态（如：LED）
   * 当接收到平台下发的属性请求后，解析其中请求的属性，并获取设备中的数据，然后响应平台
## 工程基础：
   * 正点原子 战舰开发板 基础工程
   * 正点原子 DHT11 KEY LED TIMER WDG USART DELAY 等驱动
   * cJSON库
   * 张继瑞 ESP8266 MQTT OneNet 等相关的驱动
## 数据类型：
   * DHT11 温湿度数据 int32 可读（单向：平台不可设置改数据）
   * LED 状态数据 bool 可读可写（双向：平台可设置该数据）
## 数据变化：
   * 按键按下（正点原子 战舰开发板 KEY2）LED状态翻转
   * 平台发送LED属性设置，设备根据平台改变LED状态以及状态数据
   * 环境温湿度变化导致DHT11获取到的数据改变
