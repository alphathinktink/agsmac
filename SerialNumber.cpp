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
    //s[15 - i/2] = '\0';
  }
}
//------------------------------------------------------------------------------------------
uint8_t getUniqueSerialNumber_ex(uint8_t* name)
{
  //utox8_ex(HAL_GetUIDw0(), &name[0]);
  //utox8_ex(HAL_GetUIDw1(), &name[16]);
  //utox8_ex(HAL_GetUIDw2(), &name[32]);
  utox8_ex(0x01234678, &name[0]);
  utox8_ex(0x9ABCDEF0, &name[16]);
  utox8_ex(0x12345678, &name[32]);
  return 48;
}
//------------------------------------------------------------------------------------------
String GetSerialNumber(void)
{
	uint8_t tt[49];
	if(!SerialNumber.isEmpty() && SerialNumber!="")
	{
		return SerialNumber;
	}
	memset(tt,0,sizeof(tt));
	memset(tt,65,sizeof(tt)-1);
	uint8_t res=getUniqueSerialNumber_ex((uint8_t *)tt);
	if(res==48)
	{
		SerialNumber=(char *)tt;
	}
	return SerialNumber;
}
//------------------------------------------------------------------------------------------