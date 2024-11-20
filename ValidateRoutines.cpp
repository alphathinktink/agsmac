#ifndef VALIDATE_ROUTINES_CPP
#define VALIDATE_ROUTINES_CPP

#include <Arduino.h>

String OmNomNomSD(String &Text,const String &Delim)
{
  String Res;
  int dl=Delim.length();
  int Iso=Text.indexOf(Delim);
  if(Iso==-1)
  {
    Res=Text;
    Text="";
    return Res;
  }
  Res=Text.substring(0,Iso);
  Text=Text.substring(Ido+dl);
  return Res;
}
#endif VALIDATE_ROUTINES_CPP