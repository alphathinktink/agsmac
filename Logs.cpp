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
#include "BUSYBomb.h"
//------------------------------------------------------------------------------------------
USBHostMSD msd;
mbed::FATFileSystem usb("usb");
volatile bool usb_mounted=false;
//------------------------------------------------------------------------------------------
void DataLogStart(void)
{
  BUSYBomb
  const unsigned int maxRetries = 7; // Maximum retries
  const unsigned int delayPerRetry = 25; // Delay per retry in ms
  unsigned int retries = 0;

  pinMode(PA_15, OUTPUT); // Enable the USB-A port
  msd.deinit();
  delay(200);
  digitalWrite(PA_15, LOW);
  usb_mounted=false;
  delay(200);
  msd.deinit();
  digitalWrite(PA_15, HIGH);

  // Attempt to connect to the USB drive
  while (!msd.connect() && retries < maxRetries)
  {
    lv_timer_handler(); // Handle GUI updates
    delay(delayPerRetry);
    retries++;
  }

  if (retries >= maxRetries)
  {
    Serial.println("DataLogStart: USB connection timeout.");
    return; // Exit early if no connection
  }

  // Reset retries for the next phase
  retries = 0;

  // Wait for the device to be fully connected
  while (!msd.connected() && retries < maxRetries)
  {
    lv_timer_handler(); // Handle GUI updates
    delay(delayPerRetry);
    retries++;
  }

  if (retries >= maxRetries)
  {
    Serial.println("DataLogStart: USB device failed to connect.");
    return; // Exit early if still not connected
  }

  // Attempt to mount the USB file system
  if (usb.mount(&msd) != 0)
  {
    Serial.println("DataLogStart: Failed to mount USB file system.");
    return;
  }

  usb_mounted = true;
  Serial.println("DataLogStart: USB file system mounted successfully.");
}
//------------------------------------------------------------------------------------------
void DataLog(const String &Text)
{
  BUSYBomb
  Serial.println("DataLog: Entered");
  if (!usb_mounted)
  {
    DataLogStart();
    if (!usb_mounted)
    {
      Serial.println("USB not mounted; aborting log.");
      return;
    }
  }

  String CurTime = getLocaltime();
  String Line;
  Line.reserve(CurTime.length() + Text.length() + 3); // Preallocate memory
  Line += CurTime;
  Line += "\t";
  Line += Text;
  Line += "\n";

  if (!msd.connected())
  {
      Serial.println("USB drive disconnected. Skipping log operation.");
      usb_mounted = false;
      return;
  }

  FILE *f = fopen("/usb/agsmac.log", "a+");
  if (f == NULL)
  {
    Serial.println("Failed to open log file.");
    usb_mounted=false;
    return;
  }
  size_t bytesWritten = fprintf(f, "%s", Line.c_str());
  if (bytesWritten < Line.length())
  {
    Serial.println("Failed to write full log entry.");
  }
  int flushRes=fflush(f);
  if (flushRes != 0)
  {
    Serial.println("Failed to flush log file: 0x"+String(flushRes,HEX));
  }
  if (fclose(f) != 0)
  {
    Serial.println("Failed to close log file.");
  }
  Serial.println("DataLog: Completed");
}
//------------------------------------------------------------------------------------------
String uriEncode(const String &Text)
{
  const char *safeChars = "$-_.+!*'(),?/~"; // Allowed characters
  String Res;
  char buffer[4]; // Buffer for percent-encoding
  
  for (size_t i = 0; i < Text.length(); i++)
  {
    char c = Text.charAt(i);
    // Check if the character is alphanumeric or a safe character
    if (isalnum(c) || strchr(safeChars, c))
    {
      Res += c; // Append safe character as-is
    }
    else
    {
      // Percent-encode the character
      snprintf(buffer, sizeof(buffer), "%%%02X", (unsigned char)c);
      Res += buffer;
    }
  }
  return Res;
}//------------------------------------------------------------------------------------------
void SendLogs(const String &Server)
{
  //usb.rename("/usb/agsmac.log","/usb/agsmac_topush.log");
}
//------------------------------------------------------------------------------------------
