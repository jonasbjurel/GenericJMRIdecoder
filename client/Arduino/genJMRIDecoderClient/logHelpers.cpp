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
#include "logHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Functions: Log Helper functions                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t transformLogLevelXmlStr2Int(const char* p_loglevelXmlTxt) {
    if (!strcmp(p_loglevelXmlTxt, "DEBUG-VERBOSE"))
        return DEBUG_VERBOSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-TERSE"))
        return DEBUG_TERSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-INFO"))
        return DEBUG_INFO;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-ERROR"))
        return DEBUG_ERROR;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-PANIC"))
        return DEBUG_PANIC;
    else
        return RC_GEN_ERR;
}

const char* transformLogLevelInt2XmlStr(uint8_t p_loglevelInt) {
    if (p_loglevelInt == DEBUG_VERBOSE)
        return "DEBUG-VERBOSE";
    else if (p_loglevelInt == DEBUG_TERSE)
        return "DEBUG-TERSE";
    else if (p_loglevelInt == DEBUG_INFO)
        return "DEBUG-INFO";
    else if (p_loglevelInt == DEBUG_ERROR)
        return "DEBUG-ERROR";
    else if (p_loglevelInt ==  DEBUG_PANIC)
        return "DEBUG-PANIC";
    else
        return NULL;
}