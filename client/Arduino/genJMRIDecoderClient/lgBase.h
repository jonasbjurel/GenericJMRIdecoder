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
#ifndef LGBASE_H
#define LGBASE_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "systemState.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "lgLink.h"
#include "lgSignalMast.h"
class lgLink;
class lgSignalMast;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/

typedef float							lg_property_t[8];
#define property0						0
#define property1						1
#define property2						2
#define property3						3
#define property4						4
#define property5						5
#define property6						6
#define property7						7



/*==============================================================================================================================================*/
/* Class: lgBase                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_LG_SYSNAME					0
#define XML_LG_USRNAME					1
#define XML_LG_DESC						2
#define XML_LG_LINKADDR					3
#define XML_LG_TYPE						4
#define XML_LG_SUBTYPE					5
#define XML_LG_PROPERTIES				6
#define XML_ACT_PROPERTIES				7

#define CALL_EXT(ext_p, type, method)\
		if(!strcmp(type, "SIGNAL MAST"))\
			((lgSignalMast*)ext_p)->method;\
		else\
			panic("lgBase::CALL_EXT: Non supported type - rebooting")

#define CALL_EXT_RC(ext_p, type, method)\
		rc_t EXT_RC;\
		if(!strcmp(type, "SIGNAL MAST")){\
			EXT_RC = ((lgSignalMast*)ext_p)->method;\
		}\
		else\
			panic("lgBase::CALL_EXT: Non supported type - rebooting")

class lgBase : public systemState {
public:
	//Public methods
	lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle);
	~lgBase(void);
	rc_t init(void);
	void onConfig(const tinyxml2::XMLElement* p_sensXmlElement);
	rc_t start(void);
	static void onSysStateChangeHelper(const void* p_lgBaseHandle, uint16_t p_sysState);
	void onSysStateChange(uint16_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgBaseHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgBaseHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t setSystemName(char* p_systemName, const bool p_force = false);
	rc_t getSystemName(const char* p_systemName);
	rc_t setUsrName(char* p_usrName, bool p_force = false);
	rc_t  getUsrName(const char* p_userName);
	rc_t setDesc(char* p_description, bool p_force = false);
	rc_t getDesc(const char* p_desc);
	rc_t setAddress(uint8_t p_address);
	rc_t getAddress(uint8_t* p_address);
	rc_t setNoOffLeds(uint8_t p_noOfLeds);
	rc_t getNoOffLeds(uint8_t* p_noOfLeds);
	rc_t setProperty(uint8_t p_propertyId, const char* p_propertyValue);
	rc_t getProperty(uint8_t p_propertyId, const char* p_propertyValue);
	void setDebug(const bool p_debug);
	bool getDebug(void);
	void setStripOffset(const uint16_t p_stripOffset);
	uint16_t getStripOffset(void);

	//Public data structures
	lgLink* lgLinkHandle;
	uint8_t lgAddress;
	uint8_t lgLinkNo;
	char* xmlconfig[8];
	bool debug;
	uint16_t stripOffset;

private:
	//Private methods
	//--

	//Private data structures
	void* extentionLgClassObj;
	SemaphoreHandle_t lgBaseLock;
};

#endif /*ACTBASE_H*/