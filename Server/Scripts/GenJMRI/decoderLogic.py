#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI decoder class providing the genJMRI decoder management-, supervision and configuration. genJMRI provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
import os
import sys
import time
import threading
import traceback
import xml.etree.ElementTree as ET
import xml.dom.minidom
from momResources import *
from ui import *
from lgLinkLogic import *
from satLinkLogic import *
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('mqtt', '..\\mqtt\\mqtt.py')
from mqtt import mqtt
imp.load_source('mqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from mqttTopicsNPayloads import *
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc
imp.load_source('schema', '..\\schema\\schema.py')
from schema import *
imp.load_source('parseXml', '..\\xml\\parseXml.py')
from parseXml import *
from config import *
# End Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Local internal constants
#################################################################################################################################################
# End Local internal constants
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: decoder
# Purpose: The decoder class provides decoder management-, supervision and configuration. genJMRI provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class decoder(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        childsSchemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.DECODER_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.MQTT_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.lgLinks.value = []
        self.satLinks.value = []
        self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.decoderSystemName.value = name
        else:
            self.decoderSystemName.value = "GJD-NewDecoderSysName"
        self.userName.value = "GJD-NewDecoderUsrName"
        self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
        self.decoderMqttURI.value = "no.valid.uri"
        self.mac.value = "00:00:00:00:00:00"
        self.description.value = "New decoder"
        self.item = self.win.registerMoMObj(self, self.parentItem, self.nameKey.candidateValue, DECODER, displayIcon=DECODER_ICON)
        self.decoderPendingRestart = False
        self.missedPingReq = 0
        self.supervisionActive = False
        self.restart = True
        trace.notify(DEBUG_INFO,"New decoder: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.commitAll()
        if self.demo:
            for i in range(DECODER_MAX_LG_LINKS):
                self.addChild(LIGHT_GROUP_LINK, name="GJLL-" + str(i), config=False, demo=True)
            for i in range(DECODER_MAX_SAT_LINKS):
                self.addChild(SATELITE_LINK, name=i, config=False, demo=True)

    def onXmlConfig(self, xmlConfig):
        self.setOpStateDetail(OP_CONFIG)
        try:
            decoderXmlConfig = parse_xml(xmlConfig,
                                            {"SystemName": MANSTR,
                                             "UserName": OPTSTR,
                                             "MAC": MANSTR,
                                             "URI": MANSTR,
                                             "Description": OPTSTR,
                                             "AdminState":OPTSTR
                                             }
                                        )
            self.decoderSystemName.value = decoderXmlConfig.get("SystemName")
            if decoderXmlConfig.get("UserName") != None:
                self.userName.value = decoderXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
            self.mac.value = decoderXmlConfig.get("MAC")
            self.decoderMqttURI.value = decoderXmlConfig.get("URI")
            if decoderXmlConfig.get("Description") != None:
                self.description.value = decoderXmlConfig.get("Description")
            else:
                self.description.value = ""
            if decoderXmlConfig.get("AdminState") != None:
                self.setAdmState(decoderXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Decoder, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        res = self.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + decoderXmlConfig.get("SystemName") + ", return code: " + trace.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        for lightGroupsLinkXml in xmlConfig:
            if lightGroupsLinkXml.tag == "LightgroupsLink":
                res = self.addChild(LIGHT_GROUP_LINK, config=False, configXml=lightGroupsLinkXml,demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Light group link to " + decoderXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        for sateliteLinkXml in xmlConfig:
            if sateliteLinkXml.tag == "SateliteLink":
                res = self.addChild(SATELITE_LINK, config=False, configXml=sateliteLinkXml, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add satelite link to " + decoderXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        return rc.OK

    def updateReq(self):
        self.decoderPendingRestart = True
        return self.parent.updateReq()

    def validate(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def checkSysName(self, sysName):
        return self.parent.checkSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " was reconfigured - applying the configuration")
            res = self.__setConfig()
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for decoder " + self.decoderSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " was not reconfigured, skiping re-configuration")
        self.unSetOpStateDetail(OP_INIT)
        self.unSetOpStateDetail(OP_CONFIG)
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in self.childs.value:
                res = child.commit1()
                if res != rc.OK:
                    return res
        return rc.OK
    
    def abort(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration abort()")
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in self.childs.value:
                child.abort()
        self.abortAll()
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing decoder .xml configuration")
        decoderXml = ET.Element("Decoder")
        sysName = ET.SubElement(decoderXml, "SystemName")
        sysName.text = self.decoderSystemName.value
        usrName = ET.SubElement(decoderXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(decoderXml, "Description")
        descName.text = self.description.value
        mac = ET.SubElement(decoderXml, "MAC")
        mac.text = self.mac.value
        uri = ET.SubElement(decoderXml, "URI")
        uri.text = self.decoderMqttURI.value
        if not decoder:
            adminState = ET.SubElement(decoderXml, "AdminState")
            adminState.text = self.getAdmState()[STATE_STR]
        if includeChilds:
            childs = True
            try:
                self.childs.value
            except:
                childs = False
            if childs:
                for child in self.childs.value:
                    decoderXml.append(child.getXmlConfigTree(decoder=decoder))
        return minidom.parseString(ET.tostring(decoderXml)).toprettyxml(indent="   ") if text else decoderXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name=None, config=True, configXml=None, demo=False):
        if resourceType == LIGHT_GROUP_LINK:
            self.lgLinks.append(lgLink(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=demo))
            self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
            trace.notify(DEBUG_INFO, "Light group link: " + self.lgLinks.candidateValue[-1].nameKey.candidateValue + "is being added to decoder " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.lgLinks.candidateValue[-1].nameKey.candidateValue
                res = self.lgLinks.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Light group link: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Light group link: " + self.lgLinks.value[-1].nameKey.value + " successfully added to decoder " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_lightgroupsLinkDialog(self.lgLinks.candidateValue[-1], edit=True)
                self.dialog.show()
                trace.notify(DEBUG_INFO, "Light group link: " + self.lgLinks.value[-1].nameKey.value + "is added to decoder " + self.nameKey.value)
                self.reEvalOpState()
                return rc.OK

        elif resourceType == SATELITE_LINK:
            self.satLinks.append(satLink(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=demo))
            self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
            trace.notify(DEBUG_INFO, "Satelite link: " + self.satLinks.candidateValue[-1].nameKey.candidateValue + "is being added to decoder " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.satLinks.candidateValue[-1].nameKey.candidateValue
                res = self.satLinks.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Satelite link: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Satelite link: " + self.satLinks.value[-1].nameKey.value + " successfully added to decoder " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_satLinkDialog(self.satLinks.candidateValue[-1], edit=True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Decoder could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo) + " \"replacement\": " + str(replacement))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Gen JMRI server (top decoder) only takes SATELITE_LINK and LIGHT_GROPU_LINK as childs, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return child.canDelete()
        try:
            self.lgLinks.remove(child)
        except:
            pass
        try:
            self.satLinks.remove(child)
        except:
            pass
        self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
        return rc.OK

    def view(self):
        self.dialog = UI_decoderDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_decoderDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, LIGHT_GROUP_LINK | SATELITE_LINK)
        self.dialog.show()

    def delete(self, top=False):
        if self.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + self.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return self.canDelete()
        try:
            for child in self.child.value:
                child.delete()
        except:
            pass
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq()
        return rc.OK

    def accepted(self):
        self.setOpStateDetail(OP_CONFIG)
        self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        res = self.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not configure " + nameKey + ", return code: " + rc.getErrStr(res))
            return res
        trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured from GUI")
        return rc.OK

    def rejected(self):
        self.abort()
        return rc.OK

    def getDecoderUri(self):
        return self.decoderMqttURI.value

    def decoderRestart(self):
        if self.decoderPendingRestart:
            self.decoderPendingRestart = False
            self.__decoderRestart()

    def __validateConfig(self):
        res = self.parent.checkSysName(self.decoderSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.decoderSystemName.candidateValue + " already in use")
            return res
        if len(self.lgLinks.candidateValue) > DECODER_MAX_LG_LINKS:
            trace.notify(DEBUG_ERROR, "Too many Lg links defined for decoder " + str(self.decoderSystemName.candidateValue) + ", " + str(len(self.lgLinks.candidateValue)) + "  given, " + str(DECODER_MAX_LG_LINKS) + " is maximum")
            return rc.RANGE_ERR
        linkNos = []
        for lgLink in self.lgLinks.candidateValue:
            try:
                linkNos.index(lgLink.lgLinkNo.candidateValue)
                trace.notify(DEBUG_ERROR, "LG Link No " + str(lgLink.lgLinkNo.candidateValue) + " already in use for decoder " + self.decoderSystemName.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        if len(self.satLinks.candidateValue) > DECODER_MAX_SAT_LINKS:
            trace.notify(DEBUG_ERROR, "Too many Sat links defined for decoder " + str(self.decoderSystemName.candidateValue) + ", " + str(len(self.satLinks.candidateValue)) + " given, " + str(DECODER_MAX_SAT_LINKS) + " is maximum")
            return rc.RANGE_ERR
        linkNos = []
        for satLink in self.satLinks.candidateValue:
            try:
                linkNos.index(satLink.satLinkNo.candidateValue)
                trace.notify(DEBUG_ERROR, "SAT Link No " + str(satLink.satLinkNo.candidateValue) + " already in use for decoder " + self.decoderSystemName.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        return rc.OK

    def __setConfig(self): 
        self.decoderOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_OPSTATE_TOPIC + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.decoderAdmTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_ADMSTATE_TOPIC + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CONFIGREQ_TOPIC + self.decoderMqttURI.value, self.__onDecoderConfigReq)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CONFIGREQ_TOPIC + self.decoderMqttURI.value, self.__onDecoderConfigReq)
        if self.getAdmState() == ADM_ENABLE:
            self.__startSupervision()
        return rc.OK

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()) + " anouncing current OPState and AdmState")
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.decoderOpTopic, OP_AVAIL_PAYLOAD)
        else:
            self.mqttClient.publish(self.decoderOpTopic, OP_UNAVAIL_PAYLOAD)
        if self.getAdmState() == ADM_ENABLE:
            self.mqttClient.publish(self.decoderAdmTopic, ADM_ON_LINE_PAYLOAD)
            self.__startSupervision()
        else:
            self.mqttClient.publish(self.decoderAdmTopic, ADM_OFF_LINE_PAYLOAD)
            self.__stopSupervision()

    def __decoderRestart(self):
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested will be restarted")
        self.restart = True

    def __onDecoderConfigReq(self, topic, value):
        if self.getAdmState() == ADM_DISABLE:
            trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested new configuration, but Adminstate is disabled - will not provide the configuration")
            return
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested new configuration")
        self.mqttClient.publish(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CONFIG_TOPIC + self.decoderMqttURI.value, self.parent.getXmlConfigTree(decoder=True, text=True, onlyIncludeThisChild=self))
        self.restart = False
        #How to trigger to send all opStates

    def __startSupervision(self): #Improvement request - after disabled a quarantain period should start requiring consecutive DECODER_MAX_MISSED_PINGS before bringing it back - may require proportional slack in the __supervisionTimer - eg (self.parent.decoderMqttKeepalivePeriod.value*1.1)
        if self.supervisionActive:
            return
        trace.notify(DEBUG_INFO, "Decoder supervision for " + self.nameKey.value + " is started/restarted")
        self.missedPingReq = 0
        self.supervisionActive = True
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)
        threading.Timer(self.parent.decoderMqttKeepalivePeriod.value, self.__supervisionTimer).start()

    def __stopSupervision(self):
        trace.notify(DEBUG_INFO, "Decoder supervision for " + self.nameKey.value + " is stoped")
        self.missedPingReq = 0
        self.supervisionActive = False
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)

    def __supervisionTimer(self):
        trace.notify(DEBUG_VERBOSE, "Decoder " + self.nameKey.value + " received a supervision timer")
        if self.getAdmState() == ADM_DISABLE:
            self.missedPingReq = 0
            return
        if not self.supervisionActive:
            return
        threading.Timer(self.parent.decoderMqttKeepalivePeriod.value, self.__supervisionTimer).start()
        self.missedPingReq += 1
        if self.missedPingReq >= DECODER_MAX_MISSED_PINGS:
            self.missedPingReq = DECODER_MAX_MISSED_PINGS
            if not (self.getOpStateDetail() & OP_FAIL[STATE]):
                self.setOpStateDetail(OP_FAIL)

    def __onPingReq(self, topic, payload):
        trace.notify(DEBUG_VERBOSE, "Decoder " + self.nameKey.value + " received an upstream PING request")
        if self.getAdmState() == ADM_DISABLE:
            return
        if not self.supervisionActive:
            return
        self.missedPingReq = 0
        if (self.getOpStateDetail() & OP_FAIL[STATE]):
            self.unSetOpStateDetail(OP_FAIL)
        if not self.restart:
            self.mqttClient.publish(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_DOWNSTREAM + self.decoderMqttURI.value, PING)

