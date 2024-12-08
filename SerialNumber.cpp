//------------------------------------------------------------------------------------------
#include "SerialNumber.h"
//------------------------------------------------------------------------------------------
static String SerialNumber="";
//------------------------------------------------------------------------------------------
/*static void utox8_ex(uint32_t val, uint8_t* s)
{
  for (int i = 0; i < 16; i=i+2) {
    int d = val & 0XF;
    val = (val >> 4);

    s[15 - i -1] = d > 9 ? 'A' + d - 10 : '0' + d;
    s[15 - i] = '\0';
  }
}*/
//------------------------------------------------------------------------------------------
static void utox8_ex(uint32_t val, uint8_t* s)
{
  for (int i = 0; i < 16; i+=2) {
    int d = val & 0xF;
    val = (val >> 4);

    s[15 - i/2 -1] = (d > 9)?(65 + d - 10):(48 + d);
    s[15 - i/2] = '\0';
  }
}
//------------------------------------------------------------------------------------------
uint8_t getUniqueSerialNumber_ex(uint8_t* name)
{
  //utox8_ex(HAL_GetUIDw0(), &name[0]);
  //utox8_ex(HAL_GetUIDw1(), &name[8]);
  //utox8_ex(HAL_GetUIDw2(), &name[16]);
  utox8_ex(0x01234678, &name[0]);
  utox8_ex(0x9ABCDEF0, &name[8]);
  utox8_ex(0x12345678, &name[16]);
  return 24;
}
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
