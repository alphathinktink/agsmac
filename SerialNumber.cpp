//------------------------------------------------------------------------------------------
#include "SerialNumber.h"
//------------------------------------------------------------------------------------------
static String SerialNumber="";
//------------------------------------------------------------------------------------------
String GetSerialNumber(void)
{
	char sn[25];
	if(!SerialNumber.isEmpty() && SerialNumber!="")
	{
		return SerialNumber;
	}
	memset(sn,0,sizeof(sn));
	memset(sn,65,sizeof(sn)-1);
	int res=snprintf(sn, sizeof(sn), "%08lX%08lX%08lX", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
	if(res>=24)
	{
		SerialNumber=(char *)sn;
	}
	return SerialNumber;
}
//------------------------------------------------------------------------------------------
