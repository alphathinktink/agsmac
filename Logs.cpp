//------------------------------------------------------------------------------------------
#include "Logs.h"
#include "ValidateRoutines.h"
#include <DigitalOut.h>
#include <FATFileSystem.h>
#include <Arduino_USBHostMbed5.h>
#include "lvgl.h"
#include "Times.h"
#include <WiFi.h>
#include <WiFiSSLClient.h>
//------------------------------------------------------------------------------------------
USBHostMSD msd;
mbed::FATFileSystem usb("usb");
volatile bool usb_mounted=false;
//------------------------------------------------------------------------------------------
void DataLogStart(void)
{
  pinMode(PA_15, OUTPUT); //enable the USB-A port
  digitalWrite(PA_15, HIGH);
  int err;
  unsigned int timeout=2000;
  while(!msd.connect() && timeout>0)
  {
    lv_timer_handler();
    delay(100);
    timeout-=100;
  }
  if(timeout<=0)
  {
    return;
  }
  timeout=2000;
  while (!msd.connected() && timeout>0) {
    lv_timer_handler();
    delay(500);
    timeout-=500;
  }
  if(timeout<=0)
  {
    return;
  }
  err = usb.mount(&msd);
  if (err) {
    return;
  }
  usb_mounted=true;
}
//------------------------------------------------------------------------------------------
void DataLog(const String &Text)
{
  if(!usb_mounted)
  {
    DataLogStart();
    if(!usb_mounted)
    {
      return;
    }
  };
  int err;
  mbed::fs_file_t file;
  struct dirent *ent;
  int dirIndex = 0;
  int res = 0;
  String CurTime=getLocaltime();
  String Line=CurTime+"\t"+Text+"\n";
  FILE *f = fopen("/usb/agsmac.log", "a+");
  if(f==NULL)
  {
    return;
  }
  fflush(stdout);
  fseek(f,0,SEEK_END);
  fprintf(f, Line.c_str());
  fflush(stdout);
  err = fclose(f);
  if (err < 0) {
  } else {
  }
}
//------------------------------------------------------------------------------------------
void SendLogs(const String &Server)
{
  //usb.rename("/usb/agsmac.log","/usb/agsmac_topush.log");
}
//------------------------------------------------------------------------------------------
