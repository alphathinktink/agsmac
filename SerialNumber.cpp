//------------------------------------------------------------------------------------------
#include "SerialNumber.h"
//------------------------------------------------------------------------------------------
static String SerialNumber="";
//------------------------------------------------------------------------------------------
String GetSerialNumber(void)
{
	uint8_t tt[49];
	if(!SerialNumber.isEmpty() && SerialNumber!="")
	{
		return SerialNumber;
	}
	memset(tt,0,sizeof(tt));
	uint8_t res=getUniqueSerialNumber((uint8_t *)tt);
	if(res==48)
	{
		SerialNumber=(char *)tt;
	}
	return SerialNumber;
}
//------------------------------------------------------------------------------------------
