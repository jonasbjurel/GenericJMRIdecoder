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

#ifndef PINOUT_H
#define PINOUT_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
//-
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/


/*==============================================================================================================================================*/
/* ESP32 pinout                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
const uint8_t LGLINK_PINS[] =			{25, 36};
const uint8_t SATLINK_TX_PINS[] =		{37, 38};
const uint8_t  SATLINK_RX_PINS[] =		{ 39, 40 }; //NEEDS TO BE CHECKED
#define WPS_PIN                         34
/*==============================================================================================================================================*/
/* END ESP32 pinout                                                                                                                             */
/*==============================================================================================================================================*/

#endif /*PINOUT_H*/

