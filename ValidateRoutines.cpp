#include "ValidateRoutines.h"

#include "Arduino.h"
#include "WString.h"

using namespace arduino;

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
  Text=Text.substring(Iso+dl);
  return Res;
}

bool StringIsInteger(const String &Test)
{
  if(Test=="" || Test.isEmpty())return false;
  unsigned int c=Test.length();
  for(unsigned int i=0;i<c;i++)
  {
    if(!isDigit(Test.charAt(i)))return false;
  }
  return true;
}

bool ValidateIPv4Address(const String &Text)//four dotted quads
{
  if(Text.length()>15)return false;
  if(Text=="" || Text.isEmpty())return false;
  String Test=Text;
  String P1=OmNomNomSD(Test,".");
  String P2=OmNomNomSD(Test,".");
  String P3=OmNomNomSD(Test,".");
  String P4=OmNomNomSD(Test,".");
  if(!Test.isEmpty())return false;
  if(P1=="" || P1.isEmpty() || P2=="" || P2.isEmpty() || P3=="" || P3.isEmpty() || P4=="" || P4.isEmpty())return false;
  if(!StringIsInteger(P1))return false;
  if(!StringIsInteger(P2))return false;
  if(!StringIsInteger(P3))return false;
  if(!StringIsInteger(P4))return false;
  int p1=P1.toInt();
  int p2=P2.toInt();
  int p3=P3.toInt();
  int p4=P4.toInt();
  if(p1<0 || p1>255)return false;
  if(p2<0 || p2>255)return false;
  if(p3<0 || p3>255)return false;
  if(p4<0 || p4>255)return false;
  return true;
}
