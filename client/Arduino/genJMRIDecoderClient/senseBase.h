/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonas.bjurel@hotmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law and agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*==============================================================================================================================================*/
/* END License                                                                                                                                  */
/*==============================================================================================================================================*/
#ifndef SENSBASE_H
#define SENSBASE_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "systemState.h"
#include "sat.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "senseDigital.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
class sat;
class senseDigital;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: senseBase                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_SENS_SYSNAME				0
#define XML_SENS_USRNAME				1
#define XML_SENS_DESC					2
#define XML_SENS_PORT					3
#define XML_SENS_TYPE					4
#define XML_SENS_PROPERTIES				5

#define CALL_EXT(ext_p, type, method)\
		if(!strcmp(type, "DIGITAL"))\
			((senseDigital*)ext_p)->method;\
		else\
			panic("senseBase::CALL_EXT: Non supported type - rebooting")


class senseBase : public systemState {
public:
	//Public methods
	senseBase(uint8_t p_sensPort, sat* p_satHandle);
	~senseBase(void);
	rc_t init(void);
	void onConfig(const tinyxml2::XMLElement* p_sensXmlElement);
	rc_t start(void);
	void onDiscovered(const satelite* p_sateliteLibHandle);
	static void onSystateChangeHelper(const void* p_senseBaseHandle, uint16_t p_sysState);
	void onSystateChange(uint16_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t setSystemName(char* p_sysName, bool p_force = false);
	rc_t getSystemName(const char* p_sysName);
	rc_t setUsrName(char* p_usrName, bool p_force = false);
	rc_t getUsrName(const char* p_usrName);
	rc_t setDesc(char* p_description, bool p_force = false);
	const char* getDesc(void);
	rc_t setPort(const uint8_t p_port);
	uint8_t getPort(void);
	void setDebug(bool p_debug);
	bool getDebug(void);

	//Public data structures
	sat* satHandle;
	uint8_t sensPort;
	uint8_t satAddr;
	uint8_t satLinkNo;
	bool pendingStart;
	char* xmlconfig[6];
	bool debug;

private:
	//Private methods
	//--

	//Private data structures
	const satelite* satLibHandle;
	void* extentionSensClassObj;
	SemaphoreHandle_t sensLock;
};

#endif /*SENSBASE_H*/
