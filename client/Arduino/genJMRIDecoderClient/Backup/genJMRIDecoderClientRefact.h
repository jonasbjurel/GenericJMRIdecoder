#ifndef GENJMRI_H
#define GENJMRI_H

//#include <MqttClient.h> After recovery
#include <ArduinoLog.h>
//#include <C:\Users\jonas\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WiFi\src\WiFi.h>
#include "WiFi.h"
#include "esp_wps.h"
#include "esp_timer.h"
#include <PubSubClient.h>
#include <QList.h>
#include <tinyxml2.h>
#include <esp_task_wdt.h>
#include <Adafruit_NeoPixel.h>
#include <cmath>
#include "ESPTelnet.h"
#include <SimpleCLI.h>
#include <limits>
#include <cstddef>



class wdt;
class networking;
class mqtt;
class decoder;
class lgLink;
class satLink;
class signalMastAspects;
class flash;
class mastDecoder;



//ESP32 Pinout
#define NOOF_LGLINKS                    2
#define LGLINK_PINS                     {25, 36}
#define NOOF_SATLINKS                   2
#define SATLINK_TX_PINS                 {37, 38}
#define SATLINK_RX_PINS                 {39, 40} //NEEDS TO BE CHECKED
#define WPS_PIN                         34







//Debug level
#define DEBUG_VERBOSE                   4
#define DEBUG_TERSE                     3
#define DEBUG_INFO                      2
#define DEBUG_ERROR                     1
#define DEBUG_PANIC                     0



//Top decoder parameters
#define DEFAULT_RSYSLOGRECEIVER         "syslog.jmri.local"
#define DEFAULT_LOGLEVEL                DEBUG_INFO
#define DEFAULT_NTPSERVER               "pool.ntp.org"
#define DEFAULT_TIMEZONE                0
#define DEFAULT_PINGPERIOD              1



//Signal mast params
#define MAX_LGSTRIPLEN                  32
#define SM_DIM_SLOW                     150 // Dim time in ms (0-65535)
#define SM_DIM_NORMAL                   75
#define SM_DIM_FAST                     40
#define SM_FLASH_TYPE_SLOW              0   // Flash type definitions - don't change
#define SM_FLASH_TYPE_NORMAL            1
#define SM_FLASH_TYPE_FAST              2
#define SM_FLASH_TIME_SLOW              0.5 // Signal mast flash frequency (Hz)
#define SM_FLASH_TIME_NORMAL            1
#define SM_FLASH_TIME_FAST              1.5
#define SM_BRIGHNESS_FAIL               120 // Brighness for a fully saturated mast head (0-255)
#define SM_BRIGHNESS_HIGH               80
#define SM_BRIGHNESS_NORMAL             40
#define SM_BRIGHNESS_LOW                20

//Satelite link params
#define MAX_SATELITES                   16

//Satelite params
#define MAX_ACT                         4
#define MAX_SENS                        8


// MQTT Params
#define MQTT_BUFF_SIZE                  16384
#define DEFAULT_MQTT_CLIENT_ID          "JMRI generic decoder"
#define DEFAULT_MQTT_KEEP_ALIVE         10
#define MAX_MQTT_CONNECT_ATTEMPTS_100MS 100
#define DEFAULT_MQTT_PINGPERIOD         10
#define MAX_MQTT_LOST_PINGS             3




// MQTT Topics
#define MQTT_DISCOVERY_REQUEST_TOPIC    "/trains/track/discoveryreq/"
#define MQTT_DISCOVERY_RESPONSE_TOPIC   "/trains/track/discoveryres/"
#define MQTT_PING_UPSTREAM_TOPIC        "/trains/track/decoderSupervision/upstream/"
#define MQTT_PING_DOWNSTREAM_TOPIC      "/trains/track/decoderSupervision/downstream/"
#define MQTT_CONFIG_REQ_TOPIC           "/trains/track/decoderconfigreq/"
#define MQTT_CONFIG_RESP_TOPIC          "/trains/track/decoderconfigresp/"
#define MQTT_LOG_TOPIC                  "/trains/track/log/"
#define MQTT_ASPECT_TOPIC               "/trains/track/lightgroups/lightgroup/"
#define MQTT_DECODER_OPSTATE_TOPIC      "/trains/track/opState/decoder/"
#define MQTT_LGLINK_OPSTATE_TOPIC       "/trains/track/lglink/opState/"
#define MQTT_LG_OPSTATE_TOPIC           "/trains/track/lightgroup/opState/"
#define MQTT_SATLINK_OPSTATE_TOPIC      "/trains/track/sateliteLink/opState/"
#define MQTT_SAT_OPSTATE_TOPIC          "/trains/track/satelite/opState/"
#define MQTT_SAT_OPSTATE_TOPIC          "/trains/track/satelite/opState/"
#define MQTT_DECODER_ADMSTATE_TOPIC     "/trains/track/decoder/admState/"
#define MQTT_LGLINK_ADMSTATE_TOPIC      "/trains/track/lglink/admState/"
#define MQTT_LG_ADMSTATE_TOPIC          "/trains/track/lightgroup/admState/"
#define MQTT_SATLINK_ADMSTATE_TOPIC     "/trains/track/sateliteLink/admState/"
#define MQTT_SAT_ADMSTATE_TOPIC         "/trains/track/satelite/admState/"
#define MQTT_SATLINK_STATS_TOPIC         "/trains/track/sateliteLink/statistics/"




// MQTT Payloads
#define MQTT_DISCOVERY_REQ_PAYLOAD      "<DISCOVERY_REQUEST/>"                                  //Decoder Discovey request payload
#define MQTT_CONFIG_REQ_PAYLOAD         "<CONFIGURATION_REQUEST/>"
#define MQTT_PING                       "<PING/>"                                               //MQTT Ping payload
#define MQTT_OP_AVAIL_PAYLOAD           "<OPSTATE>AVAILABLE</OPSTATE>"
#define MQTT_OP_UNAVAIL_PAYLOAD         "<OPSTATE>UNAVAILABLE</OPSTATE>"
#define MQTT_ADM_ON_LINE_PAYLOAD        "<ADMSTATE>ONLINE</ADMSTATE>"
#define MQTT_ADM_OFF_LINE_PAYLOAD       "<ADMSTATE>OFFLINE</ADMSTATE>"
#define MQTT_BOOL_TRUE_PAYLOAD          "YES"
#define MQTT_BOOL_FALSE_PAYLOAD         "NO"




// Threading
#define CORE_0                        0                                                       //Core 0 used for scheduling tasks, shared with ESP32 infrastructure real time tasks (Eg WIFI)
#define CORE_1                        1                                                       //Core 1 used for scheduling tasks, purely for applications


//Task properties                                                                              (0-24) Higher is more priority
#define FLASH_LOOP_PRIO               10
#define MQTT_POLL_PRIO                10
#define PING_TIMER_PRIO               10
#define UPDATE_STRIP_PRIO             10
#define CLI_POLL_PRIO                 5
#define CPU_PM_POLL_PRIO              10
#define CPU_SATLINK_PRIO              20
#define SATLINK_PM_PRIO               10
#define FLASH_LOOP_CORE               CORE_1
#define MQTT_POLL_CORE                CORE_1
#define PING_TIMER_CORE               CORE_1
#define UPDATE_STRIP_CORE             CORE_1
#define CLI_POLL_CORE                 CORE_1
#define CPU_PM_CORE                   CORE_1
#define CPU_SATLINK_CORE              CORE_1
#define SATLINK_PM_CORE               CORE_1

//Other Params
#define STRIP_UPDATE_MS               5                                                       //Lightgoups 2811 strip update periond [ms]
#define SATLINK_UPDATE_MS             5                                                       //Satelite linl update periond [ms]


// General types
typedef rc_t const uint8_t;
typedef ipPort_t uint8_t;


/*==============================================================================================================================================*/
/* Functions: Helper functions                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
char* createNcpystr(const char* src);
char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings);
void getTagTxt(tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len);

/*==============================================================================================================================================*/
/* END Helper functions                                                                                                                         */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
//Class fixed constants
#define CPU_HISTORY_SIZE 61                                                           //Provides samples for maximum 60 second cpu load avarage

class cpu{
  public:
    //methods
    static void init(void);
    static float getAvgCpuLoadCore(uint8_t p_core, uint8_t p_period);
    //static float get1SecCpuLoadCore(uint8_t p_core);
    //static uint8_t get10SecCpuLoadCore(uint8_t p_core);
    //static uint8_t get1MinCpuLoadCore(uint8_t p_core);
    static uint8_t getCpuMaxLoadCore(uint8_t p_core);
    static uint8_t clearCpuMaxLoadCore(uint8_t p_core);
    static void getTaskInfoAll(char* p_taskInfoTxt);
    static uint8_t getTaskInfoByTask(char* p_task, char* p_taskInfoTxt);
    static uint32_t getHeapMemInfoAll(void);
    static uint32_t getHeapMemInfoMaxAll(void);
    static uint32_t getHeapMemTrend10minAll(void);
    static uint32_t getStackByTask(char* p_task);
    static uint32_t getMaxStackByTask(char* p_task);

    //Data structures
    //--


  private:
    //methods
    static void load(void* dummy);
    static void cpuMeasurment0(void* dummy);
    static void cpuMeasurment1(void* dummy);
    static void cpuPM(void* dummy);

    //Data structures
    static SemaphoreHandle_t cpuPMLock;
    static uint64_t accBusyTime0;
    static uint64_t accTime0;
    static uint64_t accBusyTime1;
    static uint64_t accTime1;
    static uint64_t totalUsHistory0[CPU_HISTORY_SIZE];
    static uint64_t totalUsHistory1[CPU_HISTORY_SIZE];
    static uint64_t busyUsHistory0[CPU_HISTORY_SIZE];
    static uint64_t busyUsHistory1[CPU_HISTORY_SIZE];
    static uint8_t index;
    static uint32_t totIndex;
    static float maxCpuLoad0;
    static float maxCpuLoad1;
};


/*==============================================================================================================================================*/
/* Class: decoderCli                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define CLI_PARSE_CONTINUE  0
#define CLI_PARSE_STOP  1

typedef void (*cliInitCb_t)(void);
typedef uint8_t (*cliCb_t)(cmd* c);

struct cliContext_t{
  char* cliContext;
  cliInitCb_t cliContextInitCb;
  cliCb_t cliContextCb;
};

class decoderCli{
  //methods
  public:
    static void init(void);
    static void registerCliContext(cliContext_t* p_context);
    static void printToCli(String p_output, bool partial = false);
    
    //Data structures
    static Command help;
    static Command set;
    static Command get;
    static Command exec;

  private:
    //methods
    static void onTelnetConnect(String ip);
    static void onTelnetDisconnect(String ip);
    static void onTelnetReconnect(String ip);
    static void onTelnetConnectionAttempt(String ip);
    static void onTelnetString(String p_cmd);
    static void onCliCommand(cmd* p_cmd);
    static cliContext_t* getCliContext(char* p_context);
    static void setCliContext(cliContext_t* p_context);
    static void resetCliContext(void);
    static void commonCliContextInit(void);
    static uint8_t contextCommonCliCmd(cmd* p_cmd);
    static void rootCliContextInit(void);
    static uint8_t onRootCliCmd(cmd* p_cmd);
    static void cliPoll(void* dummy);

    //Data structures
    static QList<cliContext_t*>* cliContextList;
    static cliContext_t* currentCliContext;
    static cliContext_t* newCliContext;
    static SimpleCLI* cli;
    static ESPTelnet telnet;
    static cliContext_t* rootCliContext;
    static uint8_t connections;
};

/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define FAULTACTION_REBOOT                  1<<7        // Reboots the entire decoder
#define FAULTACTION_MQTTDOWN                1<<6        // Brings down MQTT
#define FAULTACTION_FREEZE                  1<<5        // Freezes the state of the decoder, inhibits MQTT bring down and reboot 
#define FAULTACTION_FAILSAFE_LGS            1<<4        // Fail safes the Light groups channels decoders
#define FAULTACTION_FAILSAFE_SENSORS        1<<3        // Fail safes the sensor decoders
#define FAULTACTION_FAILSAFE_TURNOUTS       1<<2        // Fail safes the turnout decoders
#define FAULTACTION_FAILSAFE_ALL            FAULTACTION_FAILSAFE_LGS|FAULTACTION_FAILSAFE_SENSORS|FAULTACTION_FAILSAFE_TURNOUTS // Fail safe alias for all of the decoders
#define FAULTACTION_DUMP_ALL                1<<1        // Tries to dump as much decoder information as possible
#define FAULTACTION_DUMP_LOCAL              1           // Dumps information from the object at fault

struct wdt_t {
  long unsigned int wdtTimeout;
  esp_timer_handle_t timerHandle;
  char* wdtDescription;
  uint8_t wdtAction;
};

class wdt{
  public:
    //methods
    wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction);
    ~wdt(void);
    void feed(void);

    //Data structures
    //--
    
  private:
    //methods
    static void kickHelper(wdt* p_wdtObject);
    void kick(void);

    //Data structures
    wdt_t* wdtData;
    esp_timer_create_args_t wdtTimer_args;
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/


/*==============================================================================================================================================*/
/* Class: networking                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void (*netwCallback_t)(uint8_t);

#define ESP_WPS_MODE        WPS_TYPE_PBC                            //Wifi WPS mode set to "push-button
#define ESP_MANUFACTURER    "ESPRESSIF" 
#define ESP_MODEL_NUMBER    "ESP32"
#define ESP_MODEL_NAME      "ESPRESSIF IOT"
#define ESP_DEVICE_NAME     "ESP STATION"

class networking{
  public:
    //Public methods
    static void start(void);                                      //Start ZTP WIFI
    static void regCallback(const netwCallback_t p_callback);     //Register callback for networking status changes
    static const char* getSsid(void);                             //Get connected WIFI SSID
    static uint8_t getChannel(void);                              //Get connected WIFI Channel
    static long getRssi(void);                                    //Get connected WIFI signal SNR
    static const char* getMac(void);                              //Get connected WIFI MAC adress
    static IPAddress getIpaddr(void);                             //Get host IP address received by DHCP
    static IPAddress getIpmask(void);                             //Get network IP mask received by DHCP
    static IPAddress getGateway(void);                            //Get gateway router address received by DHCP
    static IPAddress getDns(void);                                //Get DNS server address received by DHCP
    static IPAddress getNtp(void);                                //Get NTP server address received by DHCP
    static const char* getHostname(void);                         //Get hostname received by DHCP
    static uint8_t getOpState(void);                              //Get current Networking Operational state

    //Public data structures
    static uint8_t opState;                                       //Holds current Operational state

  private:
    //Private methods
    static void wpsStart(void);                                   
    static void wpsInitConfig(void);
    static String wpspin2string(uint8_t a[]);
    static void WiFiEvent(WiFiEvent_t event, system_event_info_t info);

    //Private data structures
    static String ssid;                                           //Connected WIFI SSID
    static uint8_t channel;                                       //Connected WIFI channel
    static long rssi;                                             //RSSI WIFI signal SNR
    static String mac;                                            //WIFI MAC address
    static IPAddress ipaddr;                                      //Host IP address received by DHCP
    static IPAddress ipmask;                                      //Network IP mask received by DHCP
    static IPAddress gateway;                                     //Gateway router received by DHCP
    static IPAddress dns;                                         //DNS server received by DHCP
    static IPAddress ntp;                                         //NTP server received by DHCP
    static String hostname;                                       //Hostname received by DHCP
    static netwCallback_t callback;                               //Reference to callback for network status change
    static esp_wps_config_t config;                               //Internal WIFI configuration object
};
//networking NETWORK;

/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define MQTT_POLL_PERIOD_MS 100
#define POLL_MQTT_LATENCY_AVG_TIME 10

typedef void(*mqttSubCallback_t)(const char* topic, const char* payload, const void* args);
typedef void(*mqttStatusCallback_t)(uint8_t mqttStatus);


struct mqttSub_t {
  char* topic;
  mqttSubCallback_t mqttSubCallback;
  void* mqttCallbackArgs;
};

struct mqttTopic_t {
  char* topic;
  QList<mqttSub_t*>* topicList;
};

class mqtt {
  public:
    //Public methods
    static uint8_t init(const char* p_broker, uint16_t p_port, const char* p_user, const char* p_pass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, bool p_defaultRetain);
    static uint8_t setOpstateTopic(char* p_opStateTopic, char* p_upPayload, char* p_downPayload);
    static uint8_t subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args);
    static uint8_t unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback);
    static uint8_t sendMsg(const char* p_topic, const char* p_payload, bool p_retain = defaultRetain);
    static uint8_t regStatusCallback(const mqttStatusCallback_t p_statusCallback);
    static uint8_t regSupervisionCallback(const mqttSupervisionCallback_t p_supervisionCallback);
    static uint8_t setPingPeriod(float p_pingPeriod);
    static uint8_t up(void);
    static uint8_t down(void);
    static char* getUri(void);
    static uint8_t getOpState(void);
    static uint32_t getOverRuns(void);
    static void clearOverRuns(void);
    static uint32_t getMeanLatency(void);
    static uint32_t getMaxLatency(void);
    static void clearMaxLatency(void);

    //Public data structures
    //--
    
  private:
    //Private methods
    static void discover(void);
    static void onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy);
    static void logSubscribers(void);
    static void poll(void* dummy);
    static void onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length);
    static uint8_t reSubscribe(void);
    static TaskHandle_t* supervisionTaskHandle;
    static void mqttPingTimer(void* dummy);
    static void onMqttPing(const char* p_topic, const char* p_payload, const void* p_dummy);

    //Private data structures
    static uint8_t opState;
    static uint32_t overRuns;
    static uint32_t maxLatency;
    static uint16_t avgSamples;
    static uint32_t* latencyVect;
    static WiFiClient espClient;
    static PubSubClient mqttClient;
    static SemaphoreHandle_t mqttLock;
    static char* broker;
    static uint16_t port;
    static char* uri;
    static char* user;
    static char* pass;
    static char* clientId;
    static uint8_t defaultQoS;
    static uint8_t keepAlive;
    static bool opStateTopicSet;
    static char* opStateTopic;
    static char* upPayload;
    static char* downPayload;
    static char* mqttPingUpstreamTopic;
    static uint8_t missedPings;
    static int mqttStatus;
    static uint8_t qos;
    static bool retain;
    static float pingPeriod;
    static bool discovered;
    static QList<mqttTopic_t*> mqttTopics;
    static mqttStatusCallback_t statusCallback;
};
//mqtt MQTT;

/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: decoder                                                                                                                            */
/* Purpose: The "decoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,        */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroups [Signal Masts | general Lights | sequencedLights],       */
/*          Turnouts or sensors...                                                                                                              */
/*          The "decoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
//Dont change
#define XML_DECODER_MQTT_URI                     0
#define XML_DECODER_MQTT_PORT                    1
#define XML_DECODER_MQTT_PREFIX                  2
#define XML_DECODER_MQTT_KEEPALIVEPERIOD         3
#define XML_DECODER_NTPURI                       4
#define XML_DECODER_NTPPORT                      5
#define XML_DECODER_TZ                           6
#define XML_DECODER_LOGLEVEL                     7
#define XML_DECODER_FAILSAFE                     8
#define XML_DECODER_SYSNAME                      9
#define XML_DECODER_USRNAME                      10
#define XML_DECODER_DESC                         11
#define XML_DECODER_MAC                          12
#define XML_DECODER_URI                          13

class decoder{
  public:
    //Public methods
    static uint8_t init(void);
    static void onConfigUpdate(const char* p_topic, const char* p_payload, const void* p_dummy);
    static uint8_t start(void);
    static char* getLogLevel(void);
    static char* getNtpServer(void);
    static uint8_t getTimezone(void);
    static float getMqttKeepAlivePeriod(void);
    static uint8_t getOpState(void);

    //Public data structures
    //--

  private:
    //Private methods
    static void __lightgropsCliContextInit(void);
    static uint8_t __onLightgropsCliCmd(cmd* p_cmd);
    
    //Private data structures
    static uint8_t opState;
    static char* mac;
    static char** xmlconfig;
    static char* ntpServer;
    static uint16_t ntpPort
    static uint8_t tz;
    static tinyxml2::XMLDocument* xmlConfigDoc;
    static SemaphoreHandle_t decoderLock;
    static lgLink*[NOOF_LGLINKS] lgLinks;
    static satLink* [NOOF_SATLINKS] satLinks;
    static cliContext_t* lightgropsCliContext;
};
/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lgLink                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define UPDATE_STRIP_LATENCY_AVG_TIME           10

#define XML_LGLINK_SYSNAME                      0
#define XML_LGLINK_USRNAME                      1
#define XML_LGLINK_DESC                         2
#define XML_LGLINK_LINK                         3

struct lightGroup_t {
  mastDecoder* lightGroupObj;
  lgLink* lightGroupLinkChannel;
  uint16_t lgAddr;
  uint16_t lgSeq;
  uint8_t lgNoOfLed; 
  uint16_t lgSeqOffset;
  char* lgSystemName;
  char* lgUserName;
  char* lgType;
};

struct stripLed_t {
  uint8_t currentValue = 0;
  uint8_t wantedValue = SM_BRIGHNESS_FAIL;
  uint8_t incrementValue = 0;
  bool dirty = false;
};

class lgLink{
  public:
    //Public methods
    lgLink(void);
    ~lgLink(void);
    uint8_t init(const uint8_t p_channel);
    uint8_t on_configUpdate(tinyxml2::XMLElement* p_lightgroupsXmlElement);
    uint8_t start(void);
    uint8_t updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, const uint8_t* p_wantedValueBuff, const uint16_t* p_transitionTimeBuff);
    flash* getFlashObj(uint8_t p_flashType);
    uint32_t getOverRuns(void);
    void clearOverRuns(void);
    uint8_t getOpState(void);
    int64_t getMeanLatency(void);
    int64_t getMaxLatency(void);
    void clearMaxLatency(void);
    uint32_t getMeanRuntime(void);
    uint32_t getMaxRuntime(void);
    void clearMaxRuntime(void);

    //Public data structures
    //--

  private:
    //Private methods
    static void updateStripHelper(void* p_lgsObject);
    void updateStrip(void);

    //Private data structures
    uint8_t opState;
    uint8_t channel;
    uint32_t overRuns;
    int64_t maxLatency;
    uint32_t maxRuntime;
    uint16_t avgSamples;
    int64_t* latencyVect;
    uint32_t* runtimeVect;
    uint8_t pin;
    QList<lightGroup_t*> lgList;
    QList<stripLed_t*> dirtyList;
    tinyxml2::XMLElement* lightGroupXmlElement;
    stripLed_t* stripCtrlBuff;
    Adafruit_NeoPixel* strip;
    uint8_t* stripWritebuff;
    SemaphoreHandle_t lgLinkLock = NULL;
    flash* FLASHSLOW;
    flash* FLASHNORMAL;
    flash* FLASHFAST;
};

/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                             */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Class: satLink                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_SATLINK_SYSNAME                      0
#define XML_SATLINK_USRNAME                      1
#define XML_SATLINK_DESC                         2
#define XML_SATLINK_LINK                         3


/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void(*flashCallback_t)(bool p_flashState, void* args);
#define FLASH_LATENCY_AVG_TIME        10

struct callbackSub_t {
  flashCallback_t callback;
  void* callbackArgs;
};

struct flash_t {
  QList<callbackSub_t*> callbackSubs;
  uint32_t onTime;
  uint32_t offTime;
  bool flashState;
};

class flash{
  public:
    //Public methods
    flash(float p_freq, uint8_t p_duty);
    ~flash(void);
    uint8_t subscribe(flashCallback_t p_callback, void* p_args);
    uint8_t unSubscribe(flashCallback_t cb);
    static void flashLoopStartHelper(void* p_flashObject);
    void flashLoop(void);
    int getOverRuns(void);
    void clearOverRuns(void);
    uint32_t getMeanLatency(void);
    uint32_t getMaxLatency(void);
    void clearMaxLatency(void);

    //Public data structures
    uint8_t opState;

  private:
    //Privat methods
    //--
    
    //Private data structures
    uint32_t overRuns;
    uint32_t maxLatency;
    uint16_t maxAvgSamples;
    uint32_t* latencyVect;
    flash_t* flashData;
    SemaphoreHandle_t flashLock;
};

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define SM_MAXHEADS             12
#define UNLIT_APPEARANCE        0
#define LIT_APPEARANCE          1
#define FLASH_APPEARANCE        2
#define UNUSED_APPEARANCE       3

struct mastType_t {
  char* name;
  uint8_t headAspects[SM_MAXHEADS];
  uint8_t noOfHeads;
  uint8_t noOfUsedHeads;
};

struct aspects_t {
  char* name;
  QList<mastType_t*> mastTypes;
};


class signalMastAspects{
  public:
    //Public methods
    static uint8_t onConfigUpdate(tinyxml2::XMLElement* p_smAspectsXmlElement);
    static void dumpConfig(void);
    static uint8_t getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance);
    static uint8_t getNoOfHeads(char* p_smType);
    
    //Public data structures
    //--

  private:
    //Private methods
    //--
    
    //Private data structures
    static uint8_t opState;
    static uint8_t failsafeMastAppearance[SM_MAXHEADS];
    static QList<aspects_t*> aspects;
};
//signalMastAspects SMASPECTS;

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lg                                                                                                                                    */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_LG_SYSNAME                          0
#define XML_LG_USRNAME                          1
#define XML_LG_DESC                             2
#define XML_LG_TYPE                             3
#define XML_LG_LINKADDR                         4


/*==============================================================================================================================================*/
/* Class: mastDecoder                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/


#define XML_MAST_TYPE                           0 //Property 1 in xml config
#define XML_MAST_DIM_TIME                       1 //Property 2 in xml config
#define XML_MAST_FLASH_FREQ                     2 //Property 3 in xml config
#define XML_MAST_BRIGHTNESS                     3 //Property 3 in xml config

struct mastDesc_t {
  char* smType;
  char* smDimTime;
  char* smFlashFreq;
  char* smBrightness;
};

class mastDecoder{
  public:
    //Public methods
    mastDecoder(void);          
    ~mastDecoder(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    uint8_t init(void);
    uint8_t onConfigure(lightGroup_t* p_genLgDesc, tinyxml2::XMLElement* p_mastDescXmlElement); //Returns RC_OK if successful
    uint8_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    uint8_t stop(void);
    static void onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject);
    void onAspectChange(const char* p_topic, const char* p_payload); //Mqtt callback at aspect change
    static void onFlashHelper(const bool p_flashState, void* p_flashObj);
    void onFlash(const bool p_flashState); //Flash object call back
    uint8_t getOpState(void); //Returns the Objects Operational state

    //Public data structures
    //--

  private:
    //Private methods
    uint8_t parseXmlAppearance(const char* p_aspectXml, char* p_aspect); //Parses a new aspect from *p_aspectXml and provides clear text aspect text in p_aspect, Returns RC_OK if successful

    //Private data structures
    uint8_t opState; //Operational state of the mastDecoder object
    SemaphoreHandle_t mastDecoderLock; //Mutex to protect common object datastructures from multi thread parallel access
    SemaphoreHandle_t mastDecoderReentranceLock; // Mutex to avoid re-entrance caused by new aspects or flash events
    lightGroup_t* genLgDesc; //General light group descriptor - see lgLink definitions
    mastDesc_t* mastDesc; // mastDecoder specific descriptor
    char aspect[80]; //Curren mastDecoder aspect - clear text name
    uint8_t* appearance; //Holds the Appearance signatures for all heads in the mast (LIT, UNLIT, FLASH, UNUSED..), See definitions in the mastaspect class
    uint8_t* tmpAppearance; //Temporarrily stores the appearance as received from the lgs decoder [SM_MAXHEADS]
    uint8_t* appearanceWriteBuff; //Holds the wanted raw led wanted values to be passed to the lgLink for rendering
    uint16_t* appearanceDimBuff; //Holds the Dim timing for each head in the mast to be passed to the lgLink for rendering
    uint16_t smDimTime; //Dim timing in ms for this signal mast
    uint8_t smBrightness; //Brighness 0-255
    bool flashOn; //Holds the flash state for all heads in the mast currently flashing
};

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/

#endif /*GENJMRI_H*/
