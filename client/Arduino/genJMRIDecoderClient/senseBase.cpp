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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "senseBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "senseBase (Sensor base/Stem-cell class)"                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
senseBase::senseBase(uint8_t p_sensPort, sat* p_satHandle) : systemState(this) {
    Log.notice("senseBase::senseBase: Creating senseBase stem-object for sensor port %d, on satelite adress %d, satLink %d" CR, p_sensPort, p_satHandle->getAddr(), p_satHandle->linkHandle->getLink());
    satHandle = p_satHandle;
    sensPort = p_sensPort;
    satAddr = satHandle->getAddr();
    satLinkNo = satHandle->linkHandle->getLink();
    regSysStateCb(this, &onSystateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    sensLock = xSemaphoreCreateMutex();
    pendingStart = false;
    satLibHandle = NULL;
    debug = false;
    if (sensLock == NULL)
        panic("senseBase::senseBase: Could not create Lock objects - rebooting...");
}

senseBase::~senseBase(void) {
    panic("senseBase::~senseBase: senseBase destructior not supported - rebooting...");
}

rc_t senseBase::init(void) {
    Log.notice("senseBase::init: Initializing stem-object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseBase::onConfig(const tinyxml2::XMLElement* p_sensXmlElement) {
    if (!(getOpState() & OP_UNCONFIGURED))
        panic("senseBase:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("senseBase::onConfig: sensor port %d, on satelite adress %d, satLink %d received an uverified configuration, parsing and validating it..." CR, sensPort, satAddr, satLinkNo);
    xmlconfig[XML_SENS_SYSNAME] = NULL;
    xmlconfig[XML_SENS_USRNAME] = NULL;
    xmlconfig[XML_SENS_DESC] = NULL;
    xmlconfig[XML_SENS_PORT] = NULL;
    xmlconfig[XML_SENS_TYPE] = NULL;
    xmlconfig[XML_SENS_PROPERTIES] = NULL;
    const char* sensSearchTags[6];
    sensSearchTags[XML_SENS_SYSNAME] = "SystemName";
    sensSearchTags[XML_SENS_USRNAME] = "UserName";
    sensSearchTags[XML_SENS_DESC] = "Description";
    sensSearchTags[XML_SENS_PORT] = "Port";
    sensSearchTags[XML_SENS_TYPE] = "Type";
    sensSearchTags[XML_SENS_PROPERTIES] = "Properties";
    getTagTxt(p_sensXmlElement, sensSearchTags, xmlconfig, sizeof(sensSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_SENS_SYSNAME])
        panic("senseBase::onConfig: SystemNane missing - rebooting...");
    if (!xmlconfig[XML_SENS_USRNAME])
        panic("senseBase::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SENS_DESC])
        panic("senseBase::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SENS_PORT])
        panic("senseBase::onConfig: Port missing - rebooting...");
    if (!xmlconfig[XML_SENS_TYPE])
        panic("senseBase::onConfig: Type missing - rebooting...");
    if (atoi((const char*)xmlconfig[XML_SENS_PORT]) != sensPort)
        panic("senseBase::onConfig: Port No inconsistant - rebooting...");
    Log.notice("senseBase::onConfig: System name: %s" CR, xmlconfig[XML_SENS_SYSNAME]);
    Log.notice("senseBase::onConfig: User name:" CR, xmlconfig[XML_SENS_USRNAME]);
    Log.notice("senseBase::onConfig: Description: %s" CR, xmlconfig[XML_SENS_DESC]);
    Log.notice("senseBase::onConfig: Port: %s" CR, xmlconfig[XML_SENS_PORT]);
    Log.notice("senseBase::onConfig: Type: %s" CR, xmlconfig[XML_SENS_TYPE]);
    if (xmlconfig[XML_SENS_PROPERTIES])
        Log.notice("senseBase::onConfig: Sensor type specific properties provided, will be passed to the sensor type sub-class object: %s" CR, xmlconfig[XML_SENS_PROPERTIES]);
    if (!strcmp((const char*)xmlconfig[XML_SENS_TYPE], "DIGITAL")) {
        Log.notice("senseBase::onConfig: Sensor type is digital - programing sens-stem object by creating a senseDigital extention class object" CR);
        extentionSensClassObj = (void*) new senseDigital(this);
    }
    // else if (other sensor types) {...}
    else
        panic("senseBase::onConfig: sensor type not supported");
    CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], init());
    if (xmlconfig[XML_SENS_PROPERTIES]) {
        Log.notice("senseBase::onConfig: Configuring the sensor base stem-object with properties" CR);
        CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onConfig(p_sensXmlElement->FirstChildElement("Properties")));
    }
    else
        Log.notice("senseBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("senseBase::onConfig: Configuration successfully finished" CR);
}

rc_t senseBase::start(void) {
    Log.notice("senseBase::start: Starting sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("senseBase::start: sensor port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sensPort, satAddr, satLinkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (getOpState() & OP_UNDISCOVERED) {
        Log.notice("senseBase::start: sensor port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sensPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("senseBase::start: sensor port %d, on satelite adress %d, satLink %d - starting extention class" CR, sensPort, satAddr, satLinkNo);
    CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], start());
    Log.notice("senseBase::start: Subscribing to adm- and op state topics");
    char* tmpSysName;
    getSystemName(tmpSysName);
    const char* admSubscribeTopic[5] = { MQTT_SENS_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", tmpSysName };
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), onAdmStateChangeHelper, this))
        panic("senseBase::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_SENS_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", tmpSysName };
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), onOpStateChangeHelper, this))
        panic("senseBase::start: Failed to suscribe to opState topic - rebooting...");
}

void senseBase::onDiscovered(const satelite* p_sateliteLibHandle) {
    Log.notice("senseBase::onDiscovered: sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
    satLibHandle = p_sateliteLibHandle;
}

void senseBase::onSystateChangeHelper(const void* p_senseBaseHandle, uint16_t p_sysState) {
    ((senseBase*)p_senseBaseHandle)->onSystateChange(p_sysState);
}

void senseBase::onSystateChange(uint16_t p_sysState) {
    if (!(p_sysState & OP_UNCONFIGURED)){
        CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onSysStateChange(p_sysState));
        if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
            Log.notice("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, sensPort, satAddr, satLinkNo);
        else if (p_sysState & OP_INTFAIL)
            panic("senseBase::onSystateChange: sensor port on satelite has experienced an internal error - rebooting...");
        if (p_sysState)
            Log.notice("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has received Opstate %b - doing nothing" CR, sensPort, satAddr, satLinkNo, p_sysState);
        else
            Log.notice("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has received a cleared Opstate - doing nothing" CR, sensPort, satAddr, satLinkNo);
    }
}

void senseBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onOpStateChange(p_topic, p_payload);
}

void senseBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("senseBase::onOpStateChange: sensor port %d, on satelite adress %d, satLink %d got available message from server: %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("senseBase::onOpStateChange: sensor port %d, on satelite adress %d, satLink %d got unavailable message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.error("senseBase::onOpStateChange: sensor port %d, on satelite address %d on satlink %d got an invalid availability message from server %s - doing nothing" CR, sensPort, satAddr, satLinkNo, p_payload);
}

void senseBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onAdmStateChange(p_topic, p_payload);
}

void senseBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got online message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got off-line message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.error("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got an invalid admstate message from server %s - doing nothing" CR, sensPort, satAddr, satLinkNo, p_payload);
}

rc_t senseBase::setSystemName(char* p_sysName, bool p_force) {
    Log.error("senseBase::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t senseBase::getSystemName(const char* p_sysName) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::getSystemName: cannot get System name as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_sysName = xmlconfig[XML_SENS_SYSNAME];
    return RC_OK;
}

rc_t senseBase::setUsrName(char* p_usrName, const bool p_force) {
    if (!debug || !p_force) {
        Log.error("senseBase::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::setUsrName: cannot set System name as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("senseBase::setUsrName: Setting User name to %s" CR, p_usrName);
        delete (char*)xmlconfig[XML_SENS_USRNAME];
        xmlconfig[XML_SENS_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

 rc_t senseBase::getUsrName(const char* p_usrName) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::getUsrName: cannot get User name as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_usrName = xmlconfig[XML_SENS_USRNAME];
    return RC_OK;
}

rc_t senseBase::setDesc(char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.error("senseBase::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::setDesc: cannot set Description as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("senseBase::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SENS_DESC];
        xmlconfig[XML_SENS_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* senseBase::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::getDesc: cannot get Description as sensor is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SENS_DESC];
}

rc_t senseBase::setPort(const uint8_t p_port) {
    Log.error("senseBase::setPort: cannot set port - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

uint8_t senseBase::getPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("senseBase::getPort: cannot get port as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_SENS_PORT]);
}

void senseBase::setDebug(bool p_debug) {
    debug = p_debug;
}

bool senseBase::getDebug(void) {
    return debug;
}

/*==============================================================================================================================================*/
/* END Class senseBase                                                                                                                           */
/*==============================================================================================================================================*/