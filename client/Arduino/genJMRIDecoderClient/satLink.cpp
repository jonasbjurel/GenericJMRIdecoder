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
#include "satLink.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "satLink(Satelite Link)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
satLink::satLink(uint8_t p_linkNo) : systemState(this) {
    Log.notice("satLink::satLink: Creating Satelite link channel %d" CR, p_linkNo);
    linkNo = p_linkNo;
    regSysStateCb(this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
    satLinkLock = xSemaphoreCreateMutex();
    if (satLinkLock == NULL)
        panic("satLink::satLink: Could not create Lock objects - rebooting...");

}

satLink::~satLink(void) {
    panic("satLink::~satLink: satLink destructior not supported - rebooting...");
}

rc_t satLink::init(void) {
    Log.notice("satLink::init: Initializing Satelite link channel %d" CR, linkNo);
    Log.notice("satLink::init: Creating satelites for link channel %d" CR, linkNo);
    for (uint8_t satAddress = 0; satAddress < MAX_SATELITES; satAddress++) {
        sats[satAddress] = new sat(satAddress, this);
    if (sats[satAddress] == NULL)
        panic("satLink::init: Could not create satelite object for link channel - rebooting...");
    }
    satLinkLibHandle = new sateliteLink(linkNo, (gpio_num_t)(SATLINK_TX_PINS[linkNo]),
                                        (gpio_num_t)(SATLINK_RX_PINS[linkNo]),
                                        (rmt_channel_t)(SATLINK_RMT_TX_CHAN[linkNo]),
                                        (rmt_channel_t)(SATLINK_RMT_RX_CHAN[linkNo]),
                                        SATLINK_RMT_TX_MEMBANK[linkNo],
                                        SATLINK_RMT_RX_MEMBANK[linkNo], 
                                        CPU_SATLINK_PRIO,
                                        CPU_SATLINK_CORE[linkNo],
                                        SATLINK_UPDATE_MS);
    if (satLinkLibHandle == NULL)
        panic("satLink::init: Could not create satelite link library object for link channel - rebooting...");
    satLinkLibHandle->satLinkRegSatDiscoverCb(&onDiscoveredSateliteHelper, this);
    satLinkLibHandle->satLinkRegStateCb(&onSatLinkLibStateChangeHelper, this);
    satLinkLibHandle->setErrTresh(0, 0);
    txUnderunErr = 0;
    rxOverRunErr = 0;
    scanTimingViolationErr = 0;
    rxCrcErr = 0;
    remoteCrcErr = 0;
    rxSymbolErr = 0;
    rxDataSizeErr = 0;
    wdErr = 0;
    return RC_OK;
}

void satLink::onConfig(tinyxml2::XMLElement* p_satLinkXmlElement) {
    if (!(getOpState() & OP_UNCONFIGURED))
        panic("satLink:onConfig: satLink channel received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("satLink::onConfig: satLink channel %d received an uverified configuration, parsing and validating it..." CR, linkNo);
    xmlconfig[XML_SATLINK_SYSNAME] = NULL;
    xmlconfig[XML_SATLINK_USRNAME] = NULL;
    xmlconfig[XML_SATLINK_DESC] = NULL;
    xmlconfig[XML_SATLINK_LINK] = NULL;
    const char* satLinkSearchTags[4];
    satLinkSearchTags[XML_SATLINK_SYSNAME] = "SystemName";
    satLinkSearchTags[XML_SATLINK_USRNAME] = "UserName";
    satLinkSearchTags[XML_SATLINK_DESC] = "Description";
    satLinkSearchTags[XML_SATLINK_LINK] = "Link";
    getTagTxt(p_satLinkXmlElement, satLinkSearchTags, xmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_SATLINK_SYSNAME])
        panic("satLink::onConfig: SystemName missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_USRNAME])
        panic("satLink::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_DESC])
        panic("satLink::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_LINK])
        panic("satLink::onConfig: Link missing - rebooting...");
    if (atoi(xmlconfig[XML_SATLINK_LINK]) != linkNo)
        panic("satLink::onConfig: Link no inconsistant - rebooting...");
    Log.notice("satLink::onConfig: System name: %s" CR, xmlconfig[XML_SATLINK_SYSNAME]);
    Log.notice("satLink::onConfig: User name:" CR, xmlconfig[XML_SATLINK_USRNAME]);
    Log.notice("satLink::onConfig: Description: %s" CR, xmlconfig[XML_SATLINK_DESC]);
    Log.notice("satLink::onConfig: Link: %s" CR, xmlconfig[XML_SATLINK_LINK]);
    Log.notice("satLink::onConfig: Creating and configuring signal mast aspect description object");
    Log.notice("satLink::onConfig: Configuring Satelites");
    tinyxml2::XMLElement* satXmlElement;
    satXmlElement = p_satLinkXmlElement->FirstChildElement("Satelite");
    const char* satSearchTags[8];
    char* satXmlConfig[8];
    for (uint16_t satItter = 0; false; satItter++) {
        if (satXmlElement == NULL)
            break;
        if (satItter >= MAX_SATELITES)
            panic("satLink::onConfig: > maximum sats provided - not supported, rebooting...");
            satSearchTags[XML_SAT_ADDR] = "LinkAddress";
        getTagTxt(satXmlElement, satSearchTags, satXmlConfig, sizeof(satXmlElement) / 4); // Need to fix the addressing for portability
        if (!satXmlConfig[XML_SAT_ADDR])
            panic("satLink::onConfig:: Satelite Linkaddr missing - rebooting..." CR);
        sats[atoi(satXmlConfig[XML_SAT_ADDR])]->onConfig(satXmlElement);
        addSysStateChild(sats[atoi(satXmlConfig[XML_SAT_ADDR])]);
        satXmlElement = p_satLinkXmlElement->NextSiblingElement("Satelite");
    }
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("satLink::onConfig: Configuration successfully finished" CR);
}

rc_t satLink::start(void) {
    Log.notice("satLink::start: Starting Satelite link: %d" CR, linkNo);
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("satLink::start: Satelite Link %d not configured - will not start it" CR, linkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("satLink::start: Subscribing to adm- and op state topics");
    const char* admSubscribeTopic[5] = { MQTT_SATLINK_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), &onAdmStateChangeHelper, this))
        panic("satLink::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_SATLINK_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), &onOpStateChangeHelper, this))
        panic("satLink::start: Failed to suscribe to opState topic - rebooting...");
    for (uint16_t satItter = 0; satItter < MAX_SATELITES; satItter++) {
        sats[satItter]->start();
    }
    char satlinkPmTaskName[30];
    sprintf(satlinkPmTaskName, CPU_SATLINK_PM_TASKNAME, linkNo);
    if (rc_t rc = satLinkLibHandle->enableSatLink())
        panic("satLink::start: could not enable satelite link  - rebooting...");
    xTaskCreatePinnedToCore(
        pmPollHelper,                                                                   // Task function
        satlinkPmTaskName,                                                              // Task function name reference
        CPU_SATLINK_PM_STACKSIZE_1K * 1024,                                             // Stack size
        this,                                                                           // Parameter passing
        CPU_SATLINK_PM_PRIO,                                                            // Priority 0-24, higher is more
        NULL,                                                                           // Task handle
        ((linkNo % MAX_SATELITES) ? CPU_SATLINK_PM_CORE[0] : CPU_SATLINK_PM_CORE[1]));    // Core [CORE_0 | CORE_1]
    unSetOpState(OP_INIT);
    Log.notice("lgLink::start: lightgroups link %d and all its lightgroupDecoders have started" CR, linkNo);
    return RC_OK;
}

void satLink::onDiscoveredSateliteHelper(satelite* p_sateliteLibHandle, uint8_t p_satLink, uint8_t p_satAddr, bool p_exists, void* p_satLinkHandle) {
    if (p_satLink != ((satLink*)p_satLinkHandle)->linkNo)
        panic("satLink::onDiscoveredSateliteHelper: Inconsistant link number - Rebooting...");
    ((satLink*)p_satLinkHandle)->sats[p_satAddr]->onDiscovered(p_sateliteLibHandle, p_satAddr, p_exists);
}

void satLink::pmPollHelper(void* metaData_p) {
    Log.notice("satLink::pmPollHelper: Starting PM polling for satLink %d" CR, ((satLink*)metaData_p)->getLink());
    int64_t  nextLoopTime = esp_timer_get_time();
    int64_t  thisLoopTime;
    TickType_t delay;
    while (true) {
        thisLoopTime = nextLoopTime;
        nextLoopTime += 1000000; //1E6 uS = 1S
        ((satLink*)metaData_p)->onPmPoll();

        for (uint8_t satItter = 0; satItter < MAX_SATELITES; satItter++)
            ((satLink*)metaData_p)->sats[satItter]->onPmPoll();
        if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0)
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
    }
}

void satLink::onPmPoll(void) {
    satPerformanceCounters_t pmData;
    satLinkLibHandle->getSatStats(&pmData, true);
    txUnderunErr += pmData.txUnderunErr;
    rxOverRunErr += pmData.rxOverRunErr;
    scanTimingViolationErr += pmData.scanTimingViolationErr;
    rxCrcErr += pmData.rxCrcErr;
    remoteCrcErr += pmData.remoteCrcErr;
    rxSymbolErr += pmData.rxSymbolErr;
    rxDataSizeErr += pmData.rxDataSizeErr;
    wdErr += pmData.wdErr;
    const char* publishPmTopic[5] = { MQTT_SATLINK_STATS_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    char publishPmPayload[150];
    sprintf(publishPmPayload, "<statReport>\n"
        "<rxCrcErr>%d</rxCrcErr>\n"
        "<remCrcErr> %d</remCrcErr>\n"
        "<rxSymErr> %d</rxSymErr>\n"
        "<rxSizeErr> %d</rxSizeErr>\n"
        "<wdErr> %d</wdErr>\n"
        "</statReport>",
        pmData.rxCrcErr,
        pmData.remoteCrcErr,
        pmData.rxSymbolErr,
        pmData.rxDataSizeErr,
        pmData.wdErr);
    if (mqtt::sendMsg(concatStr(publishPmTopic, 5), publishPmPayload, false))
        Log.error("satLink::onPmPoll: Failed to send PM report" CR);
}

void satLink::onSatLinkLibStateChangeHelper(sateliteLink* p_sateliteLinkLibHandler, uint8_t p_linkAddr, satOpState_t p_satOpState, void* p_satLinkHandler) {
    ((satLink*)p_satLinkHandler)->onSatLinkLibStateChange(p_satOpState);
}

void satLink::onSatLinkLibStateChange(const satOpState_t satOpState_p) {
    if (!(getOpState() & OP_INIT)) {
        if (satOpState_p)
            setOpState(OP_INTFAIL);
        else
            unSetOpState(OP_INTFAIL);
    }
}

void satLink::onSysStateChangeHelper(const void* p_satLinkHandle, uint16_t p_sysState) {
    ((satLink*)p_satLinkHandle)->onSysStateChange(p_sysState);
}

void satLink::onSysStateChange(uint16_t p_sysState) {
    if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
        Log.notice("satLink::onSystateChange: satelite link %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, linkNo);
    else if (p_sysState & OP_INTFAIL)
        panic("satLink::onSystateChange: satelite link has experienced an internal error - rebooting...");
    if (p_sysState)
        Log.notice("satLink::onSystateChange: satelite link %d has received Opstate %b - doing nothing" CR, linkNo, p_sysState);
    else
        Log.notice("lgLink::onSystateChange: satelite link %d has received a cleared Opstate - doing nothing" CR, linkNo);
}

void satLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onOpStateChange(p_topic, p_payload);
}

void satLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("satLink::onOpStateChange: got available message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("satLink::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.error("satLink::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void satLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void satLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("satLink::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("satLink::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.error("satLink::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

rc_t satLink::setSystemName(const char* p_systemName, const bool p_force) {
    Log.error("satLink::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* satLink::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::getSystemName: cannot get System name as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_SYSNAME];
}

rc_t satLink::setUsrName(const char* p_usrName, const bool p_force) {
    if (!debug || !p_force) {
        Log.error("satLink::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::setUsrName: cannot set System name as satLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("satLink::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_SATLINK_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* satLink::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::getUsrName: cannot get User name as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_USRNAME];
}

rc_t satLink::setDesc(const char* p_description, const bool p_force) {
    if (!debug || !p_force) {
        Log.error("satLink::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::setDesc: cannot set Description as satLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("satLink::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SATLINK_DESC];
        xmlconfig[XML_SATLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* satLink::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::getDesc: cannot get Description as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_DESC];
}

const rc_t satLink::setLink(const char* p_link) {
    Log.error("satLink::setLink: cannot set Link No - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const rc_t satLink::getLink(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::getLink: cannot get Link No as satLink is not configured" CR);
        return RC_GEN_ERR;
    }
    return atoi(xmlconfig[XML_SATLINK_LINK]);
}

const void satLink::setDebug(const bool p_debug) {
    debug = p_debug;
}

const bool satLink::getDebug(void) {
    return debug;
}

uint32_t satLink::getTxUnderuns(void) {
    return txUnderunErr;
}

void satLink::clearTxUnderuns(void) {
    txUnderunErr = 0;
}

uint32_t satLink::getRxOverRuns(void) {
    return rxOverRunErr;
}

void satLink::clearRxOverRuns(void) {
    rxOverRunErr = 0;
}

uint32_t satLink::getscanTimingViolations(void) {
    return scanTimingViolationErr;
}

void satLink::clearScanTimingViolations(void) {
    scanTimingViolationErr = 0;
}

uint32_t satLink::getRxCrcErrs(void) {
    return rxCrcErr;
}

void satLink::clearRxCrcErrs(void) {
    rxCrcErr = 0;
}

uint32_t satLink::getRemoteCrcErrs(void) {
    return rxCrcErr;
}

void satLink::clearRemoteCrcErrs(void) {
    rxCrcErr = 0;
}

uint32_t satLink::getRxSymbolErrs(void) {
    return rxSymbolErr;
}

void satLink::clearRxSymbolErrs(void) {
    rxSymbolErr = 0;
}

uint32_t satLink::getRxDataSizeErrs(void) {
    return rxDataSizeErr;
}

void satLink::clearRxDataSizeErrs(void) {
    rxDataSizeErr = 0;
}

uint32_t satLink::getWdErrs(void) {
    return wdErr;
}

void satLink::clearWdErrs(void) {
    wdErr = 0;
}

//int64_t lgLink::getMeanLatency(void) {}
//int64_t lgLink::getMaxLatency(void) {}
//void lgLink::clearMaxLatency(void) {}
// uint32_t lgLink::getMeanRuntime(void) {}
//uint32_t lgLink::getMaxRuntime(void) {}
//void lgLink::clearMaxRuntime(void) {}

/*==============================================================================================================================================*/
/* END Class satLink                                                                                                                            */
/*==============================================================================================================================================*/