//------------------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------------------
#include "Arduino.h"
#include "WString.h"

using namespace arduino;
//------------------------------------------------------------------------------------------
String getLocaltime(void);
void setNtpTime(const String &timeServer);
unsigned long sendNTPpacket(const char * address);
unsigned long parseNtpPacket(void);
//------------------------------------------------------------------------------------------
