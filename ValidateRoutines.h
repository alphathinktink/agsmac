#pragma once

#include "Arduino.h"
#include "WString.h"

using namespace arduino;

String OmNomNomSD(String &Text,const String &Delim);
bool StringIsInteger(const String &Test);
bool ValidateIPv4Address(const String &Text);//four dotted quads
