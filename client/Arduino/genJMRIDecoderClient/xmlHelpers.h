/*============================================================================================================================================= */
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonasbjurel@hotmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
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

#ifndef XMLHLP_H
#define XMLHLP_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries/tinyxml2/tinyxml2.h"
#include "logHelpers.h"
#include <ArduinoLog.h>
#include "rc.h"


/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Helper: xmlHelpers                                                                                                                           */
/* Purpose: Provides helper functions for XML handling                                                                                          */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
void getTagTxt(const tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len);


/*==============================================================================================================================================*/
/* END xmlHelpers                                                                                                                               */
/*==============================================================================================================================================*/
#endif /*XMLHLP_H*/
