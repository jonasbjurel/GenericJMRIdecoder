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
#include "actMem.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actMem (actuator memory DNA class)"                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actMem::actMem(actBase* p_actBaseHandle, const char* p_type, char* p_subType) {
    actBaseHandle = p_actBaseHandle;
    if (!strcmp(p_subType, MQTT_MEM_SOLENOID_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_SOLENOID;
    }
    if (!strcmp(p_subType, MQTT_MEM_SERVO_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_SERVO;
    }
    if (!strcmp(p_subType, MQTT_MEM_PWM100_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PWM100;
    }
    if (!strcmp(p_subType, MQTT_MEM_PWM1_25K_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PWM125K;
    }
    else if (!strcmp(p_subType, MQTT_MEM_ONOFF_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_ONOFF;
    }
    else if (!strcmp(p_subType, MQTT_MEM_PULSE_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PULSE;
    }
    actPort = actBaseHandle->actPort;
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->satHandle->getSystemName();
    satLibHandle = NULL;
    pendingStart = false;
    actMemSolenoidPushPort = true;
    actMemSolenoidActivationTime = ACTMEM_DEFAULT_SOLENOID_ACTIVATION_TIME_MS;
    sysState = OP_INIT | OP_UNCONFIGURED;
    debug = false;
    Log.notice("actMem::actMem: Creating memory extention object for %s on actuator port %d, on satelite adress %d, satLink %d" CR, p_subType, actPort, satAddr, satLinkNo);
    actMemLock = xSemaphoreCreateMutex();
    if (actMemLock == NULL)
        panic("actMem::actMem: Could not create Lock objects - rebooting...");
    actMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    orderedActMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    actMemFailsafePos = atoi(ACTMEM_DEFAULT_FAILSAFE);
}

actMem::~actMem(void) {
    panic("actMem::~actMem: actMem destructor not supported - rebooting...");
}

rc_t actMem::init(void) {
    Log.notice("actMem::init: Initializing actMem actuator extention object for memory %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actMem::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("actMem::onConfig: Did not expect any configuration for memory actuator extention object - rebooting...");
}

rc_t actMem::start(void) {
    Log.notice("actMem::start: Starting actMem actuator extention object %s, on actuator port% d, on satelite adress% d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->getOpState() & OP_UNCONFIGURED) {
        Log.notice("actMem::start: actMem actuator extention object %s, on actuator port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sysName, actPort, satAddr, satLinkNo);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (actBaseHandle->getOpState() & OP_UNDISCOVERED) {
        Log.notice("actMem::start: actMem actuator extention class object %s, on actuator port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sysName, actPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("actMem::start: Configuring and startings actMem extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);

    if (actMemType == ACTMEM_TYPE_SOLENOID) {
        if (actPort % 2) {
            actMemSolenoidPushPort = false;
            Log.notice("actMem::start: Startings solenoid memory actuator pull port %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        }
        else {
            Log.notice("actMem::start: Startings solenoid memory actuator push port %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
            actMemSolenoidPushPort = true;
        }
        satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
    }

    else if (actMemType == ACTMEM_TYPE_SERVO) {
        Log.notice("actMem::start: Startings servo memory  actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
    }

    else if (actMemType == ACTMEM_TYPE_PWM100) {
        Log.notice("actMem::start: Startings PWM 100 Hz memory  actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
    }

    else if (actMemType == ACTMEM_TYPE_PWM125K) {
        Log.notice("actMem::start: Startings PWM 125 KHz memory actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        satLibHandle->setSatActMode(SATMODE_PWM1_25K, actPort);
    }

    else if (actMemType == ACTMEM_TYPE_ONOFF) {
        Log.notice("actMem::start: Startings ON/OFF memory actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    }
    setFailSafe(true);
    Log.notice("actMem::start: Subscribing to memory orders for Memory actuator %s", sysName);
    const char* actMemActOrders[3] = { MQTT_MEM_TOPIC, "/", sysName };
    if (mqtt::subscribeTopic(concatStr(actMemActOrders, 3), &onActMemChangeHelper, this))
        panic("actMem::start: Failed to suscribe to memory actuator order topic - rebooting...");
    return RC_OK;
}

void actMem::onDiscovered(satelite* p_sateliteLibHandle) {
    satLibHandle = p_sateliteLibHandle;
    Log.notice("actMem::onDiscovered: actMem extention class object %s, on actuator port %s, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->pendingStart) {
        Log.notice("actMem::onDiscovered: Initiating pending start for actMem extention class object %s, on actuator port %d, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
        start();
    }
}

void actMem::onSysStateChange(uint16_t p_sysState) {
    sysState = p_sysState;
    Log.notice("actMem::onSystateChange: Got a new systemState %d for actMem extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysState, actPort, satAddr, satLinkNo);
    if (sysState)
        setFailSafe(true);
    else
        setFailSafe(false);
}

void actMem::onActMemChangeHelper(const char* p_topic, const char* p_payload, const void* p_actMemHandle) {
    ((actMem*)p_actMemHandle)->onActMemChange(p_topic, p_payload);
}

void actMem::onActMemChange(const char* p_topic, const char* p_payload) {
    Log.notice("actMem::onMemActChange: Got a change order for memory actuator %s - new value %d" CR, sysName, p_payload);

    if (strcmp(p_topic, "ON")) {
        orderedActMemPos = 255;
        if (!failSafe)
            actMemPos = 255;
    }
    else if (strcmp(p_topic, "OFF")) {
        orderedActMemPos = 0;
        if (!failSafe)
            actMemPos = 0;
    }
    else {
        orderedActMemPos = atoi(p_topic);
        if (!failSafe)
            actMemPos = orderedActMemPos;
    }
    setActMem();
}

void actMem::setActMem(void) {
    if (actMemType == ACTMEM_TYPE_SOLENOID) {
        if ((actMemPos) ^ actMemSolenoidPushPort) {
            if (satLibHandle->setSatActVal(actMemSolenoidActivationTime, actPort))
                Log.error("actMem::setActMem: Failed to execute order for memory solenoid actuator %s" CR, sysName);
            Log.notice("actMem::setActMem: Memory solenoid actuator change order for %s fininished" CR, sysName);
        }
    }
    else if (actMemType == ACTMEM_TYPE_SERVO) {
        uint8_t tmpServoPwmPos = SERVO_LEFT_PWM_VAL + (actMemPos * (SERVO_RIGHT_PWM_VAL - SERVO_LEFT_PWM_VAL) / 256);
        if (satLibHandle->setSatActVal(tmpServoPwmPos, actPort))
            Log.error("actMem::setActMem: Failed to execute order for memory servo actuator %s" CR, sysName);
        Log.notice("actMem::setActMem: Memory servo actuator change order for %s fininished" CR, sysName);
    }
    else if (actMemType == ACTMEM_TYPE_PWM100) {
        if (satLibHandle->setSatActVal(actMemPos, actPort))
            Log.error("actMem::setActMem: Failed to execute order for memory PWM 100 Hz actuator %s" CR, sysName);

        Log.notice("actMem::setActMem: Memory 100 Hz PWM actuator change order for %s fininished" CR, sysName);
    }
    else if (actMemType == ACTMEM_TYPE_PWM125K) {
        if (satLibHandle->setSatActVal(actMemPos, actPort))
            Log.error("actMem::setActMem: Failed to execute order for memory PWM 125 KHz actuator %s" CR, sysName);
        Log.notice("actMem::setActMem: Memory 125 KHz PWM actuator change order for %s fininished" CR, sysName);
    }
    else if (actMemType == ACTMEM_TYPE_ONOFF) {
        if (actMemPos) {
            if (satLibHandle->setSatActMode(SATMODE_HIGH, actPort))
                Log.error("actMem::setActMem: Failed to execute order for memory ON/OFF actuator %s" CR, sysName);
        }
        else {
            if (satLibHandle->setSatActMode(SATMODE_LOW, actPort))
                Log.error("actMem::setActMem: Failed to execute order for memory ON/OFF actuator %s" CR, sysName);
        }
        Log.notice("actMem::setActMem: Memory ON/OFF actuator change order for %s fininished" CR, sysName);
    }
}

void actMem::setFailSafe(bool p_failSafe) {
    failSafe = p_failSafe;
    if (failSafe) {
        Log.notice("actMem::setFailSafe: Fail-safe set for memory actuator %s" CR, sysName);
        actMemPos = actMemFailsafePos;
    }
    else {
        Log.notice("actMem::setFailSafe: Fail-safe un-set for memory actuator %s" CR, sysName);
        actMemPos = orderedActMemPos;
    }
    setActMem();
}

void actMem::setDebug(bool p_debug) {
    Log.notice("actMem::setDebug: Debug mode set for memory %s" CR, sysName);
    debug = p_debug;
}

bool actMem::getDebug(void) {
    return debug;
}

/*==============================================================================================================================================*/
/* END Class actMem                                                                                                                             */
/*==============================================================================================================================================*/