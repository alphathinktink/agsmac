//------------------------------------------------------------------------------------------
#include "Times.h"
#include "ValidateRoutines.h"
#include <WiFiUdp.h>
#include "mbed.h"
#include <mbed_mktime.h>
#include "lvgl.h"
//------------------------------------------------------------------------------------------
unsigned int localPort = 2390;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP Udp;
//------------------------------------------------------------------------------------------
String getLocaltime(void)
{
  char buffer[32];
  tm t;
  _rtc_localtime(time(NULL), &t, RTC_4_YEAR_LEAP_YEAR_SUPPORT);
  //lv_label_set_text(label,"11/22/2024 7:27 PM");
  strftime(buffer, 32, "%m/%d/%Y %I:%M:%S%p", &t);
  String Res=String(buffer);
  Res+=" UTC";
  return Res;
}
//------------------------------------------------------------------------------------------
bool setNtpTime(const String &timeServer)
{
  Udp.begin(localPort);
  unsigned long res=sendNTPpacket(timeServer.c_str());
  if(res==0)
  {
    Udp.stop();
    return false;
  }
  for(int i=0;i<1000;i+=100)
  {
    lv_timer_handler();
    delay(100);
  }
  res=parseNtpPacket();
  Udp.stop();
  if(res==0)return false;
  return true;
}
//------------------------------------------------------------------------------------------
unsigned long sendNTPpacket(const char * address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0; // Stratum, or type of clock
  packetBuffer[2] = 6; // Polling Interval
  packetBuffer[3] = 0xEC; // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  if(Udp.beginPacket(address, 123)==0)return 0; // NTP requests are to port 123
  if(Udp.write(packetBuffer, NTP_PACKET_SIZE)==0)
  {
    Udp.clearWriteError();
    Udp.endPacket();
    return 0;
  }
  if(Udp.endPacket()==0)return 0;
  return 1;
}
//------------------------------------------------------------------------------------------
unsigned long parseNtpPacket(void)
{
  if (!Udp.parsePacket())
    return 0;

  Udp.read(packetBuffer, NTP_PACKET_SIZE);
  const unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  const unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  const unsigned long secsSince1900 = highWord << 16 | lowWord;
  constexpr unsigned long seventyYears = 2208988800UL;
  const unsigned long epoch = secsSince1900 - seventyYears;
  set_time(epoch);
  return epoch;
}
