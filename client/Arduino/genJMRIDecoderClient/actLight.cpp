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
#include "actLight.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actLight (actuator light DNA class)"                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actLight::actLight(actBase* p_actBaseHandle, const char* p_type, char* p_subType) {
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->actPort;
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->satHandle->getSystemName();
    satLibHandle = NULL;
    pendingStart = false;
    sysState = OP_INIT | OP_UNCONFIGURED;
    debug = false;
    Log.notice("actLight::actLight: Creating light extention object on actuator port %d, on satelite adress %d, satLink %d" CR, actPort, satAddr, satLinkNo);
    actLightLock = xSemaphoreCreateMutex();
    if (actLightLock == NULL)
        panic("actLight::actLight: Could not create Lock objects - rebooting...");
    actLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    orderedActLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    actLightFailsafePos = ACTLIGHT_DEFAULT_FAILSAFE;
}

actLight::~actLight(void) {
    panic("actLight::~actLight: actLight destructor not supported - rebooting...");
}

rc_t actLight::init(void) {
    Log.notice("actLight::init: Initializing actLight actuator extention object for light %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actLight::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("actLight::onConfig: Did not expect any configuration for light actuator extention object - rebooting...");
}

rc_t actLight::start(void) {
    Log.notice("actLight::start: Starting actLight actuator extention object %s, on actuator port% d, on satelite adress% d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->getOpState() & OP_UNCONFIGURED) {
        Log.notice("actLight::start: actLight actuator extention object %s, on actuator port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sysName, actPort, satAddr, satLinkNo);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (actBaseHandle->getOpState() & OP_UNDISCOVERED) {
        Log.notice("actLight::start: actLight actuator extention class object %s, on actuator port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sysName, actPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("actLight::start: Configuring and startings actLight extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    setFailSafe(true);
    Log.notice("actLight::start: Subscribing to light orders for light actuator %s", sysName);
    const char* actLightOrders[3] = { MQTT_LIGHT_TOPIC, "/", sysName };
    if (mqtt::subscribeTopic(concatStr(actLightOrders, 3), &onActLightChangeHelper, this))
        panic("actLight::start: Failed to suscribe to light actuator order topic - rebooting...");
    return RC_OK;
}

void actLight::onDiscovered(satelite* p_sateliteLibHandle) {
    satLibHandle = p_sateliteLibHandle;
    Log.notice("actLight::onDiscovered: actLight extention class object %s, on actuator port %s, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->pendingStart) {
        Log.notice("actLight::onDiscovered: Initiating pending start for actLight extention class object %s, on actuator port %d, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
        start();
    }
}

void actLight::onSysStateChange(uint16_t p_sysState) {
    sysState = p_sysState;
    Log.notice("actLight::onSystateChange: Got a new systemState %d for lightMem extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysState, actPort, satAddr, satLinkNo);
    if (sysState)
        setFailSafe(true);
    else
        setFailSafe(false);
}

void actLight::onActLightChangeHelper(const char* p_topic, const char* p_payload, const void* p_actLightHandle) {
    ((actLight*)p_actLightHandle)->onActLightChange(p_topic, p_payload);
}

void actLight::onActLightChange(const char* p_topic, const char* p_payload) {
    Log.notice("actLight::onMemActChange: Got a change order for light actuator %s - new value %d" CR, sysName, p_payload);

    if (strcmp(p_topic, "ON")) {
        orderedActLightPos = 255;
        if (!failSafe)
            actLightPos = 255;
    }
    else if (strcmp(p_topic, "OFF")) {
        orderedActLightPos = 0;
        if (!failSafe)
            actLightPos = 0;
    }
    setActLight();
}

void actLight::setActLight(void) {
    if (actLightPos) {
        if (satLibHandle->setSatActMode(SATMODE_HIGH, actPort))
            Log.error("actLight::setActLight: Failed to execute order for light actuator %s" CR, sysName);
    }
    else {
        if (satLibHandle->setSatActMode(SATMODE_LOW, actPort))
            Log.error("actLight::setActLight: Failed to execute order for light actuator %s" CR, sysName);
    }
    Log.notice("actLight::setActLight: Light actuator change order for %s fininished" CR, sysName);
}

void actLight::setFailSafe(bool p_failSafe) {
    failSafe = p_failSafe;
    if (failSafe) {
        Log.notice("actLight::setFailSafe: Fail-safe set for light actuator %s" CR, sysName);
        actLightPos = actLightFailsafePos;
    }
    else {
        Log.notice("actLight::setFailSafe: Fail-safe un-set for light actuator %s" CR, sysName);
        actLightPos = orderedActLightPos;
    }
    setActLight();
}

void actLight::setDebug(bool p_debug) {
    Log.notice("actLight::setDebug: Debug mode set for light %s" CR, sysName);
    debug = p_debug;
}

bool actLight::getDebug(void) {
    return debug;
}

/*==============================================================================================================================================*/
/* END Class actLight                                                                                                                             */
/*==============================================================================================================================================*/