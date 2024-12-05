
/*
  agsmac by Leonard W. Bogard

  created 11/13/2024

  use version 4.1.5 of the [Arduino Mbed OS Giga Boards] library.
  use version 8.3.11 of the [lvgl] library.
  use version 0.3.1 of the [Arduino_USBHostMbed5] library.


*/

#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

#include "lvgl.h"

#include "KVStore.h"
#include "kvstore_global_api.h"
#include "mbed.h"

//#include <SPI.h>
#include <WiFi.h>

#include "ValidateRoutines.h"
#include "Logs.h"
#include "Times.h"

Arduino_H7_Video          Display(800, 480, GigaDisplayShield); /* Arduino_H7_Video Display(1024, 768, USBCVideo); */
Arduino_GigaDisplayTouch  TouchDetector;

WiFiClient WiFi_client;

typedef void (*WiFiConfig4Callback)(bool IsBack);
typedef void (*WiFiConfig3Callback)(bool IsBack);
typedef void (*WiFiConfig2Callback)(bool IsBack);
typedef void (*WiFiConfig1Callback)(bool IsCancel);
enum WiFi_IP_Method_enum_t{WiFi_IP_Method_DHCP=0,WiFi_IP_Method_Static=1};

static String Debug_EventCodeToString(lv_event_code_t code);
constexpr unsigned long printInterval { 1000 };
unsigned long printNow {};
String timeServer("pool.ntp.org");

bool saveSetting(const String &Key,const String &Val)
{
  int ret = kv_set(Key.c_str(), Val.c_str(), Val.length(), 0);
  if (ret != MBED_SUCCESS) {
    return false;
  } else {
    return true;
  }
}

String loadSetting(const String &Key)
{
  char buffer[256];  // Adjust size as needed
  memset(buffer,0,sizeof(buffer));
  size_t actual_size;
  int ret = kv_get(Key.c_str(), buffer, sizeof(buffer)-1, &actual_size);
  if (ret != MBED_SUCCESS) {
    return "";
  } else {
    String Res=((char *)buffer);
    return Res;
  }
}

lv_obj_t * DateTimeDisplay_lbl=NULL;

struct WiFi_scan_list_parameters
{
  String SSID;
  wl_enc_type encryptionType;
};
const char * EncryptionTypeDisplayStrings[]={"WEP","WPA/TKIP","WPA2/CCMP","WPA3/GCMP","None","Unknown","Auto"};
const wl_enc_type EncryptionTypeDisplayMap[]={ENC_TYPE_WEP,ENC_TYPE_WPA,ENC_TYPE_WPA2,ENC_TYPE_WPA3,ENC_TYPE_NONE,ENC_TYPE_UNKNOWN,ENC_TYPE_AUTO};
const int EncryptionTypeToCBIMap[]={5,5,1,5,2,0,3,4,6,5};

const char * WiFi_IP_Method_DisplayStrings[]={"DHCP(Automatic)","Static (Manual)"};
const WiFi_IP_Method_enum_t WiFi_IP_Method_DisplayMap[]={WiFi_IP_Method_DHCP,WiFi_IP_Method_Static};
const int WiFi_IP_Method_ToCBIMap[]={0,1};

lv_obj_t * WiFi_Config_Display_obj=NULL;
lv_obj_t * WiFi_scan_list=NULL;
lv_obj_t * WiFi_Rescan_btn=NULL;
lv_obj_t * WiFi_SSID_ta=NULL;
lv_obj_t * WiFi_Pass_ta=NULL;
lv_obj_t * WiFi_kb=NULL;
lv_obj_t * WiFi_SSID_label;
lv_obj_t * WiFi_Next1_btn=NULL;
lv_obj_t * WiFi_Back1_btn=NULL;
lv_obj_t * WiFi_Next3_btn=NULL;
lv_obj_t * WiFi_Back3_btn=NULL;
lv_obj_t * WiFi_EncType_dd=NULL;
lv_obj_t * WiFi_WepKeyIndex_dd=NULL;
lv_obj_t * WiFi_WepKeyIndex_lbl=NULL;
lv_obj_t * WiFi_IP_Method_dd=NULL;
lv_obj_t * WiFi_Static_IP_lbl=NULL;
lv_obj_t * WiFi_Static_IP_ta=NULL;
lv_obj_t * WiFi_Static_Netmask_lbl=NULL;
lv_obj_t * WiFi_Static_Netmask_ta=NULL;
lv_obj_t * WiFi_Static_Gateway_lbl=NULL;
lv_obj_t * WiFi_Static_Gateway_ta=NULL;
lv_obj_t * NTP_Server_ta=NULL;
lv_obj_t * NTP_Server_kb=NULL;

String WiFi_SSID="";
wl_enc_type WiFi_encryptionType=ENC_TYPE_AUTO;
String WiFi_Pass="";
int WiFi_keyIndex=1;
volatile int WiFi_status=WL_IDLE_STATUS;
WiFi_IP_Method_enum_t WiFi_IP_Method=WiFi_IP_Method_DHCP;
String WiFi_Static_IP="";
String WiFi_Static_Netmask="";
String WiFi_Static_Gateway="";

wl_enc_type WiFi_ConfigTemp_encryptionType=ENC_TYPE_AUTO;
String WiFi_ConfigTemp_SSID="";
String WiFi_ConfigTemp_Pass="";
int WiFi_ConfigTemp_keyIndex=1;
WiFi_IP_Method_enum_t WiFi_ConfigTemp_IP_Method=WiFi_IP_Method_DHCP;
String WiFi_ConfigTemp_Static_IP="";
String WiFi_ConfigTemp_Static_Netmask="";
String WiFi_ConfigTemp_Static_Gateway="";

void WiFi_config4_callback_mbox_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  lv_obj_t *current_target=lv_event_get_current_target(event);
  void *user_data=target->user_data;
  void *current_user_data=current_target->user_data;

  switch(code)
  {
    case LV_EVENT_VALUE_CHANGED:
    if(current_target!=target)
    {
      uint32_t id=lv_btnmatrix_get_selected_btn(target);
      switch(id)
      {
        case 0://Abort
        lv_msgbox_close(current_target);
        DisplayMainStatusPanel(WiFi_Config_Display_obj);
        return;
        case 1://Start Over
        lv_msgbox_close(current_target);
        lv_obj_clean(WiFi_Config_Display_obj);
        DisplayWiFiConfig1(WiFi_Config_Display_obj,WiFi_SSID,WiFi_config1_callback);
        return;
        case 2://Apply
        lv_msgbox_close(current_target);
        #define narf(x) WiFi_##x = WiFi_ConfigTemp_##x;
        narf(encryptionType)
        narf(SSID)
        narf(Pass)
        narf(keyIndex)
        narf(IP_Method)
        narf(Static_IP)
        narf(Static_Netmask)
        narf(Static_Gateway)
        #undef narf
        saveSetting("WiFi_encryptionType",String((int)WiFi_encryptionType));
        saveSetting("WiFi_SSID",WiFi_SSID);
        saveSetting("WiFi_Pass",WiFi_Pass);
        saveSetting("WiFi_keyIndex",String((int)WiFi_keyIndex));
        saveSetting("WiFi_IP_Method",String((int)WiFi_IP_Method));
        saveSetting("WiFi_Static_IP",WiFi_Static_IP);
        saveSetting("WiFi_Static_Netmask",WiFi_Static_Netmask);
        saveSetting("WiFi_Static_Gateway",WiFi_Static_Gateway);
        DisplayMainStatusPanel(WiFi_Config_Display_obj);
        break;
      }
    }
    break;
  }
}

void WiFi_config4_callback(bool IsBack)
{
  lv_refr_now(NULL);
  lv_obj_clean(WiFi_Config_Display_obj);
  lv_obj_t *ip_info_table=lv_table_create(WiFi_Config_Display_obj);
  #define narf(t,r) lv_table_set_cell_value(ip_info_table,r,0,t);
  narf("Mac Address",0);
  narf("IP",1);
  narf("Netmask",2);
  narf("Gateway",3);
  #undef narf
  #define narf(t,r) lv_table_set_cell_value(ip_info_table,r,1,t);
  String macAddress=WiFi.macAddress();
  macAddress.toUpperCase();
  narf(macAddress.c_str(),0);
  narf(WiFi.localIP().toString().c_str(),1);
  narf(WiFi.subnetMask().toString().c_str(),2);
  narf(WiFi.gatewayIP().toString().c_str(),3);
  #undef narf
  if(IsBack)
  {
    static const char * btnsF[]={"Abort","Start Over",""};
    lv_obj_t * mbox=lv_msgbox_create(NULL, "WiFi Connection", "Failed", btnsF, true);
    lv_obj_t * lv_mbox_btns=lv_msgbox_get_btns(mbox);
    lv_obj_align(mbox,LV_ALIGN_RIGHT_MID,0,-20);
    lv_obj_add_event_cb(mbox,WiFi_config4_callback_mbox_event_cb,LV_EVENT_VALUE_CHANGED/*LV_EVENT_ALL*/,NULL);
  }
  else
  {
    static const char * btnsS[]={"Abort","Start Over","Apply",""};
    lv_obj_t * mbox=lv_msgbox_create(NULL, "WiFi Connection", "Success", btnsS, true);
    lv_obj_t * lv_mbox_btns=lv_msgbox_get_btns(mbox);
    lv_obj_align(mbox,LV_ALIGN_RIGHT_MID,0,-20);
    lv_obj_add_event_cb(mbox,WiFi_config4_callback_mbox_event_cb,LV_EVENT_VALUE_CHANGED/*LV_EVENT_ALL*/,NULL);
  }
}

static void WiFi_SSID_ta_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_FOCUSED:
    case LV_EVENT_PRESSED:
    case LV_EVENT_CLICKED:
    case LV_EVENT_LONG_PRESSED:
    case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_keyboard_set_textarea(WiFi_kb,WiFi_SSID_ta);
    lv_obj_clear_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_DEFOCUSED:
    lv_keyboard_set_textarea(WiFi_kb,NULL);
    lv_obj_add_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
  }
}

static void WiFi_Pass_ta_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_FOCUSED:
    case LV_EVENT_PRESSED:
    case LV_EVENT_CLICKED:
    case LV_EVENT_LONG_PRESSED:
    case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_keyboard_set_textarea(WiFi_kb,WiFi_Pass_ta);
    lv_obj_clear_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_DEFOCUSED:
    lv_keyboard_set_textarea(WiFi_kb,NULL);
    lv_obj_add_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
  }
}

static void Password_Show_checkbox_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_VALUE_CHANGED:
    lv_obj_t *target=lv_event_get_target(event);
    lv_state_t state=lv_obj_get_state(target);
    if(state & LV_STATE_CHECKED)
    {
      lv_textarea_set_password_mode(WiFi_Pass_ta,false);
    }
    else
    {
      lv_textarea_set_password_mode(WiFi_Pass_ta,true);
    }
    return;
  }
}

static void Next2_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_CLICKED:
    lv_obj_t *target=lv_event_get_target(event);
    void *user_data=target->user_data;
    if(user_data)
    {
      WiFiConfig2Callback callback=(WiFiConfig2Callback)user_data;
      if(WiFi_SSID_ta!=NULL)
      {
        WiFi_ConfigTemp_SSID=lv_textarea_get_text(WiFi_SSID_ta);
      }
      if(WiFi_Pass_ta!=NULL)
      {
        WiFi_ConfigTemp_Pass=lv_textarea_get_text(WiFi_Pass_ta);
      }
      if(WiFi_WepKeyIndex_dd!=NULL)
      {
        WiFi_ConfigTemp_keyIndex=lv_dropdown_get_selected(WiFi_WepKeyIndex_dd)+1;
      }
      wl_enc_type encryptionType=ENC_TYPE_AUTO;
      if(WiFi_EncType_dd!=NULL)
      {
        WiFi_ConfigTemp_encryptionType=EncryptionTypeDisplayMap[lv_dropdown_get_selected(WiFi_EncType_dd)];
      }
      callback(false);
    }
    return;
  }
}

static void Back2_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_CLICKED:
    lv_obj_t *target=lv_event_get_target(event);
    void *user_data=target->user_data;
    if(user_data)
    {
      WiFiConfig2Callback callback=(WiFiConfig2Callback)user_data;
      if(WiFi_SSID_ta!=NULL)
      {
        WiFi_ConfigTemp_SSID=lv_textarea_get_text(WiFi_SSID_ta);
      }
      WiFi_ConfigTemp_Pass="";
      callback(true);
    }
    return;
  }
}

lv_obj_t * WiFi_scan_current_btn=NULL;
WiFi_scan_list_parameters * WiFi_scan_current_slp=NULL;


void DisplayWiFiConfig1_Scan(void)
{
  WiFi_scan_current_btn=NULL;
  WiFi_scan_current_slp=NULL;
  WiFiConfig1Callback callback=(WiFiConfig1Callback)(WiFi_scan_list->user_data);
  lv_label_set_text(WiFi_SSID_label,"");
  lv_obj_add_state(WiFi_Rescan_btn, LV_STATE_DISABLED);
  lv_obj_add_state(WiFi_Next1_btn, LV_STATE_DISABLED);
  lv_obj_clean(WiFi_scan_list);
  lv_refr_now(NULL);
  int8_t numNetworks=WiFi.scanNetworks();
  for(int8_t i=0;i<numNetworks;i++)
  {
    String SSID=WiFi.SSID(i);
    if(SSID.isEmpty())continue;
    if(strcmp(SSID.c_str(),"")==0)continue;
    String Caption;
    wl_enc_type encryptionType=(wl_enc_type)(WiFi.encryptionType(i));
    String EType=EncryptionTypeDisplayStrings[EncryptionTypeToCBIMap[encryptionType]];
    //String EType(encryptionType);
    Caption=SSID+" ["+EType+"]";
    WiFi_scan_list_parameters *slp=new WiFi_scan_list_parameters;
    slp->SSID=SSID;
    slp->encryptionType=encryptionType;
    lv_obj_t *btn=lv_list_add_btn(WiFi_scan_list,LV_SYMBOL_WIFI,Caption.c_str());
    btn->user_data=(void *)slp;
    lv_obj_add_event_cb(btn,(lv_event_cb_t)WiFi_scan_list_btn_event_cb,LV_EVENT_ALL,NULL);
  }
  wl_enc_type encryptionType=(wl_enc_type)0;
  WiFi_scan_list_parameters *slp=new WiFi_scan_list_parameters;
  slp->SSID="";
  slp->encryptionType=encryptionType;
  lv_obj_t *btn=lv_list_add_btn(WiFi_scan_list,LV_SYMBOL_WIFI,"");
  btn->user_data=(void *)slp;
  lv_obj_add_event_cb(btn,(lv_event_cb_t)WiFi_scan_list_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_clear_state(WiFi_Rescan_btn, LV_STATE_DISABLED);
}

void WiFi_scan_list_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  WiFi_scan_list_parameters *slp=(WiFi_scan_list_parameters *)user_data;
  switch(code)
  {
    case LV_EVENT_DELETE:
    if(slp!=NULL)
    {
      target=NULL;
      delete slp;
    }
    return;
    case LV_EVENT_CLICKED:
    if(WiFi_scan_current_btn!=NULL && WiFi_scan_current_btn!=target)
    {
      lv_obj_clear_state(WiFi_scan_current_btn,LV_STATE_PRESSED);
    }
    if(slp!=NULL)
    {
      lv_obj_add_state(target,LV_STATE_PRESSED);
      lv_obj_clear_state(WiFi_Next1_btn, LV_STATE_DISABLED);
      WiFi_scan_current_btn=target;
      WiFi_scan_current_slp=slp;
      lv_label_set_text(WiFi_SSID_label,slp->SSID.c_str());
    }
    return;
  }
}

void WiFi_Rescan_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  switch(code)
  {
    case LV_EVENT_CLICKED:
    DisplayWiFiConfig1_Scan();
    return;
  }
}

void WiFi_Back1_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  WiFiConfig1Callback callback=(WiFiConfig1Callback)user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    if(callback)
    {
      WiFi_ConfigTemp_encryptionType=ENC_TYPE_AUTO;
      WiFi_ConfigTemp_SSID="";
      WiFi_ConfigTemp_Pass="";
      WiFi_ConfigTemp_keyIndex=0;
      callback(true);
    }
    return;
  }
}

void WiFi_Next1_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  WiFiConfig1Callback callback=(WiFiConfig1Callback)user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    if(callback)
    {
      String SSID=lv_label_get_text(WiFi_SSID_label);
      wl_enc_type encryptionType=ENC_TYPE_AUTO;
      if(WiFi_scan_current_slp)
      {
        encryptionType=WiFi_scan_current_slp->encryptionType;
      }
      WiFi_ConfigTemp_SSID=SSID;
      WiFi_ConfigTemp_encryptionType=encryptionType;
      callback(false);
    }
    return;
  }
}

void DisplayWiFiConfig1(lv_obj_t *obj,const String &Current_SSID,WiFiConfig1Callback callback)
{
  //LV_SYMBOL_WIFI
  lv_obj_t *label;

  WiFi_scan_list = lv_list_create(obj);
  lv_obj_set_size(WiFi_scan_list, 350, 391);
  WiFi_scan_list->user_data=(void *)callback;

  WiFi_Rescan_btn=lv_btn_create(obj);
  label=lv_label_create(WiFi_Rescan_btn);
  lv_label_set_text(label,"Rescan");
  lv_obj_set_x(WiFi_Rescan_btn,355);
  lv_obj_set_y(WiFi_Rescan_btn,0);

  WiFi_SSID_label=lv_label_create(obj);
  lv_label_set_text(WiFi_SSID_label,"");
  lv_obj_set_x(WiFi_SSID_label,355);
  lv_obj_set_y(WiFi_SSID_label,95);

  WiFi_Next1_btn=lv_btn_create(obj);
  label=lv_label_create(WiFi_Next1_btn);
  lv_label_set_text(label,"Next");
  lv_obj_add_state(WiFi_Next1_btn, LV_STATE_DISABLED);
  WiFi_Next1_btn->user_data=(void *)callback;

  WiFi_Back1_btn=lv_btn_create(obj);
  label=lv_label_create(WiFi_Back1_btn);
  lv_label_set_text(label,"Cancel");
  WiFi_Back1_btn->user_data=(void *)callback;

  lv_obj_set_x(WiFi_Back1_btn,355);
  lv_obj_set_y(WiFi_Back1_btn,120);

  lv_obj_align_to(WiFi_Next1_btn,WiFi_Back1_btn,LV_ALIGN_OUT_RIGHT_MID,5,0);

  lv_obj_add_event_cb(WiFi_Rescan_btn,(lv_event_cb_t)WiFi_Rescan_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Back1_btn,(lv_event_cb_t)WiFi_Back1_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Next1_btn,(lv_event_cb_t)WiFi_Next1_btn_event_cb,LV_EVENT_ALL,NULL);

  DisplayWiFiConfig1_Scan();
}

void WiFi_EncType_dd_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_READY:
    lv_obj_add_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_VALUE_CHANGED:
    case LV_EVENT_CANCEL:
    int selectedEncType=lv_dropdown_get_selected(WiFi_EncType_dd);
    if(EncryptionTypeDisplayMap[selectedEncType]!=ENC_TYPE_WEP)
    {
      lv_obj_add_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
      lv_obj_clear_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
}

void DisplayWiFiConfig2(lv_obj_t *obj,uint8_t encryptionType,bool Disable_SSID,const String &Current_SSID,const String &Current_Pass, int CurrentKeyIndex /*1-4*/,WiFiConfig2Callback callback)
{
  lv_obj_t * label;

  if(!Disable_SSID)
  {
    label=lv_label_create(obj);
    lv_label_set_text(label,"Security Type:");
    lv_obj_set_width(label,700);
    lv_obj_align(label,LV_ALIGN_OUT_TOP_MID,0,0);

    WiFi_EncType_dd=lv_dropdown_create(obj);
    int c=sizeof(EncryptionTypeDisplayStrings)/sizeof(char *);
    String dd_list;
    for(int i=0;i<c;i++)
    {
      if(i>0)
      {
        dd_list+="\n";
      }
      dd_list+=EncryptionTypeDisplayStrings[i];
    }
    lv_dropdown_set_options(WiFi_EncType_dd,dd_list.c_str());
    lv_dropdown_set_selected(WiFi_EncType_dd,EncryptionTypeToCBIMap[ENC_TYPE_AUTO]);
    lv_obj_align_to(WiFi_EncType_dd,label,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    
    label=lv_label_create(obj);
    lv_label_set_text(label,"SSID:");
    lv_obj_set_width(label,700);
    //lv_obj_align(label,LV_ALIGN_OUT_TOP_MID,0,0);
    lv_obj_align_to(label,WiFi_EncType_dd,LV_ALIGN_OUT_BOTTOM_LEFT,0,10);
    
    WiFi_SSID_ta = lv_textarea_create(obj);
    lv_textarea_set_one_line(WiFi_SSID_ta,true);
    lv_textarea_set_password_mode(WiFi_SSID_ta,false);
    lv_textarea_set_max_length(WiFi_SSID_ta,32);
    lv_textarea_set_accepted_chars(WiFi_SSID_ta,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
    lv_obj_align_to(WiFi_SSID_ta,label,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    lv_obj_set_width(WiFi_SSID_ta,700);
    if(Current_SSID!=NULL)
    {
      lv_textarea_set_text(WiFi_SSID_ta,Current_SSID.c_str());
    }
  }
  else
  {
    WiFi_SSID_ta=NULL;
    WiFi_EncType_dd=NULL;
  }
  
  label=lv_label_create(obj);
  lv_label_set_text(label,"Password:");
  lv_obj_set_width(label,340);
  if(Disable_SSID)
  {
    lv_obj_align(label,LV_ALIGN_OUT_TOP_MID,0,0);
  }
  else
  {
    lv_obj_align_to(label,WiFi_SSID_ta,LV_ALIGN_OUT_BOTTOM_LEFT,0,10);
  }

  WiFi_Pass_ta = lv_textarea_create(obj);
  lv_textarea_set_one_line(WiFi_Pass_ta,true);
  lv_textarea_set_password_mode(WiFi_Pass_ta,true);
  lv_textarea_set_max_length(WiFi_Pass_ta,63);
  lv_textarea_set_accepted_chars(WiFi_Pass_ta," @`!Aa\"Bb#Cc$Dd%Ee&Ff'Gg(Hh)Ii*Jj+Kk,Ll-Mm.Nn/Oo0Pp1Qq2Rr3Ss4Tt5Uu6Vv7Ww8Xx9Yy:Zz;[{<\\|=]}>^~?_");
  lv_obj_align_to(WiFi_Pass_ta,label,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  lv_obj_set_width(WiFi_Pass_ta,340);
  if(Current_Pass!=NULL)
  {
    lv_textarea_set_text(WiFi_Pass_ta,Current_Pass.c_str());
  }

  WiFi_WepKeyIndex_lbl=lv_label_create(obj);
  lv_label_set_text(WiFi_WepKeyIndex_lbl,"Key Index:");
  lv_obj_set_width(WiFi_WepKeyIndex_lbl,340);
  lv_obj_align_to(WiFi_WepKeyIndex_lbl,label,LV_ALIGN_OUT_RIGHT_MID,10,0);

  WiFi_WepKeyIndex_dd=lv_dropdown_create(obj);
  lv_dropdown_set_options(WiFi_WepKeyIndex_dd,"1\n2\n3\n4");
  lv_dropdown_set_selected(WiFi_WepKeyIndex_dd,CurrentKeyIndex-1);
  lv_obj_align_to(WiFi_WepKeyIndex_dd,WiFi_WepKeyIndex_lbl,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  //lv_obj_set_width(WiFi_Pass_ta,340);

  if(Disable_SSID && encryptionType==ENC_TYPE_WEP)
  {
    lv_obj_clear_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
  }
  else if(Disable_SSID && encryptionType!=ENC_TYPE_WEP)
  {
    lv_obj_add_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
  }
  else if(!Disable_SSID && encryptionType==ENC_TYPE_WEP)
  {
    lv_obj_clear_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(WiFi_WepKeyIndex_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_WepKeyIndex_dd, LV_OBJ_FLAG_HIDDEN);
  }


  lv_obj_t * Password_Show_checkbox = lv_checkbox_create(obj);
  lv_checkbox_set_text(Password_Show_checkbox,"Show password.");
  lv_obj_align_to(Password_Show_checkbox,WiFi_Pass_ta,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);

  lv_obj_t * Next_btn=lv_btn_create(obj);
  label=lv_label_create(Next_btn);
  lv_label_set_text(label, "Next");
  lv_obj_center(Next_btn);
  lv_obj_align_to(Next_btn,WiFi_WepKeyIndex_dd,LV_ALIGN_OUT_BOTTOM_RIGHT,0,5);
  Next_btn->user_data=(void *)callback;

  lv_obj_t * Back_btn=lv_btn_create(obj);
  label=lv_label_create(Back_btn);
  lv_label_set_text(label, "Back");
  lv_obj_center(Back_btn);
  lv_obj_align_to(Back_btn,Next_btn,LV_ALIGN_OUT_LEFT_MID,-5,0);
  Back_btn->user_data=(void *)callback;

  WiFi_kb = lv_keyboard_create(obj);
  lv_obj_add_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);

  //lv_keyboard_set_textarea(WiFi_kb,WiFi_SSID_ta);
  if(!Disable_SSID)
  {
    lv_obj_add_event_cb(WiFi_SSID_ta, (lv_event_cb_t)WiFi_SSID_ta_event_cb,LV_EVENT_ALL,NULL);
  }
  lv_obj_add_event_cb(WiFi_Pass_ta, (lv_event_cb_t)WiFi_Pass_ta_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(Password_Show_checkbox, (lv_event_cb_t)Password_Show_checkbox_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(Next_btn,(lv_event_cb_t)Next2_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(Back_btn,(lv_event_cb_t)Back2_btn_event_cb,LV_EVENT_ALL,NULL);
  if(!Disable_SSID)
  {
    lv_obj_add_event_cb(WiFi_EncType_dd,(lv_event_cb_t)WiFi_EncType_dd_event_cb,LV_EVENT_ALL,NULL);
  }
}

void WiFi_config1_callback(bool IsCancel)
{
  if(IsCancel)
  {
    DisplayMainStatusPanel(WiFi_Config_Display_obj);
    return;
  }
  lv_obj_clean(WiFi_Config_Display_obj);
  bool DontNeedSSID=true;
  if(WiFi_ConfigTemp_SSID.isEmpty() || strcmp(WiFi_ConfigTemp_SSID.c_str(),"")==0)
  {
    DontNeedSSID=false;
  }
  DisplayWiFiConfig2(WiFi_Config_Display_obj,WiFi_ConfigTemp_encryptionType,DontNeedSSID,WiFi_ConfigTemp_SSID,WiFi_ConfigTemp_Pass,WiFi_ConfigTemp_keyIndex,WiFi_config2_callback);
}

void WiFi_config2_callback(bool IsBack)
{
  if(IsBack)
  {
    lv_obj_clean(WiFi_Config_Display_obj);
    DisplayWiFiConfig1(WiFi_Config_Display_obj,WiFi_SSID,WiFi_config1_callback);
    return;
  }
  else
  {
    lv_obj_clean(WiFi_Config_Display_obj);
    lv_refr_now(NULL);
    DisplayWiFiConfig3(WiFi_Config_Display_obj,WiFi_ConfigTemp_IP_Method,WiFi_ConfigTemp_Static_IP,WiFi_ConfigTemp_Static_Netmask,WiFi_ConfigTemp_Static_Gateway,WiFi_config3_callback);
    return;
  }
}

void WiFi_config3_callback(bool IsBack)
{
  lv_refr_now(NULL);
  lv_obj_clean(WiFi_Config_Display_obj);
  bool DontNeedSSID=true;
  if(WiFi_ConfigTemp_SSID.isEmpty() || strcmp(WiFi_ConfigTemp_SSID.c_str(),"")==0)
  {
    DontNeedSSID=false;
  }
  if(IsBack)
  {
    DisplayWiFiConfig2(WiFi_Config_Display_obj,WiFi_ConfigTemp_encryptionType,DontNeedSSID,WiFi_ConfigTemp_SSID,WiFi_ConfigTemp_Pass,WiFi_ConfigTemp_keyIndex,WiFi_config2_callback);
  }
  else
  {
    DisplayWiFiConfig4(WiFi_Config_Display_obj,WiFi_config4_callback);
  }
}

void WiFi_Back3_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  WiFiConfig3Callback callback=(WiFiConfig3Callback)user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    if(callback)
    {
      callback(true);
    }
    break;
  }
}

void WiFi_Next3_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  WiFiConfig3Callback callback=(WiFiConfig3Callback)user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    int selectedIPMethod=lv_dropdown_get_selected(WiFi_IP_Method_dd);
    WiFi_ConfigTemp_IP_Method=WiFi_IP_Method_DisplayMap[selectedIPMethod];
    if(WiFi_ConfigTemp_IP_Method!=WiFi_IP_Method_Static)
    {
      WiFi_ConfigTemp_Static_IP="";
      WiFi_ConfigTemp_Static_Netmask="";
      WiFi_ConfigTemp_Static_Gateway="";
    }
    else
    {
      WiFi_ConfigTemp_Static_IP=lv_textarea_get_text(WiFi_Static_IP_ta);
      WiFi_ConfigTemp_Static_Netmask=lv_textarea_get_text(WiFi_Static_Netmask_ta);
      WiFi_ConfigTemp_Static_Gateway=lv_textarea_get_text(WiFi_Static_Gateway_ta);
    }
    if(callback)
    {
      callback(false);
    }
    break;
  }
}

void WiFi_IP_Method_dd_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_READY:
    lv_obj_add_flag(WiFi_Static_IP_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_IP_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Netmask_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Netmask_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Gateway_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Gateway_ta, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_VALUE_CHANGED:
    case LV_EVENT_CANCEL:
    int selectedIPMethod=lv_dropdown_get_selected(WiFi_IP_Method_dd);
    if(WiFi_IP_Method_DisplayMap[selectedIPMethod]!=WiFi_IP_Method_Static)
    {
      lv_obj_add_flag(WiFi_Static_IP_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_Static_IP_ta, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_Static_Netmask_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_Static_Netmask_ta, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_Static_Gateway_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFi_Static_Gateway_ta, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
      lv_obj_clear_flag(WiFi_Static_IP_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_Static_IP_ta, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_Static_Netmask_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_Static_Netmask_ta, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_Static_Gateway_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFi_Static_Gateway_ta, LV_OBJ_FLAG_HIDDEN);
    }
    DisplayWiFiConfig3_CheckValid();
    return;
  }
}

void WiFi_Static_All_ta_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_FOCUSED:
    case LV_EVENT_PRESSED:
    case LV_EVENT_CLICKED:
    case LV_EVENT_LONG_PRESSED:
    case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_keyboard_set_textarea(WiFi_kb,target);
    lv_obj_clear_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_DEFOCUSED:
    lv_keyboard_set_textarea(WiFi_kb,NULL);
    lv_obj_add_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_VALUE_CHANGED:
    DisplayWiFiConfig3_CheckValid();
    break;
  }
}

void DisplayWiFiConfig3_CheckValid(void)
{
  bool IsValid=false;
  int selectedIPMethod=lv_dropdown_get_selected(WiFi_IP_Method_dd);
  if(WiFi_IP_Method_DisplayMap[selectedIPMethod]==WiFi_IP_Method_Static)
  {
    String p1=lv_textarea_get_text(WiFi_Static_IP_ta);
    String p2=lv_textarea_get_text(WiFi_Static_Netmask_ta);
    String p3=lv_textarea_get_text(WiFi_Static_Gateway_ta);
    IsValid=ValidateIPv4Address(p1);
    IsValid&=ValidateIPv4Address(p2);
    IsValid&=ValidateIPv4Address(p3);
  }
  else IsValid=true;
  if(IsValid)
  {
    lv_obj_clear_state(WiFi_Next3_btn, LV_STATE_DISABLED);
  }
  else
  {
    lv_obj_add_state(WiFi_Next3_btn, LV_STATE_DISABLED);
  }
}

void DisplayWiFiConfig3(lv_obj_t *obj,WiFi_IP_Method_enum_t IP_Method,const String &Static_IP,const String &Static_Netmask,const String &Static_Gateway,WiFiConfig3Callback callback)
{
  lv_obj_t *label;
  
  label=lv_label_create(obj);
  lv_label_set_text(label,"IP Assignment Method:");

  WiFi_IP_Method_dd=lv_dropdown_create(obj);
  int c=sizeof(WiFi_IP_Method_DisplayStrings)/sizeof(char *);
  String dd_list;
  for(int i=0;i<c;i++)
  {
    if(i>0)
    {
      dd_list+="\n";
    }
    dd_list+=WiFi_IP_Method_DisplayStrings[i];
  }
  lv_dropdown_set_options(WiFi_IP_Method_dd,dd_list.c_str());
  lv_dropdown_set_selected(WiFi_IP_Method_dd,WiFi_IP_Method_ToCBIMap[IP_Method]);
  lv_obj_align_to(WiFi_IP_Method_dd,label,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);

  WiFi_Static_IP_lbl=lv_label_create(obj);
  lv_label_set_text(WiFi_Static_IP_lbl,"Static IP:");
  lv_obj_align_to(WiFi_Static_IP_lbl,WiFi_IP_Method_dd,LV_ALIGN_OUT_BOTTOM_LEFT,0,5);

  WiFi_Static_IP_ta=lv_textarea_create(obj);
  lv_textarea_set_one_line(WiFi_Static_IP_ta,true);
  lv_textarea_set_password_mode(WiFi_Static_IP_ta,false);
  lv_textarea_set_max_length(WiFi_Static_IP_ta,15);
  lv_textarea_set_accepted_chars(WiFi_Static_IP_ta,"0123456789.:");
  lv_obj_align_to(WiFi_Static_IP_ta,WiFi_Static_IP_lbl,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  lv_textarea_set_text(WiFi_Static_IP_ta,Static_IP.c_str());

  WiFi_Static_Netmask_lbl=lv_label_create(obj);
  lv_label_set_text(WiFi_Static_Netmask_lbl,"Netmask:");
  lv_obj_align_to(WiFi_Static_Netmask_lbl,WiFi_Static_IP_ta,LV_ALIGN_OUT_BOTTOM_LEFT,0,5);

  WiFi_Static_Netmask_ta=lv_textarea_create(obj);
  lv_textarea_set_one_line(WiFi_Static_Netmask_ta,true);
  lv_textarea_set_password_mode(WiFi_Static_Netmask_ta,false);
  lv_textarea_set_max_length(WiFi_Static_Netmask_ta,15);
  lv_textarea_set_accepted_chars(WiFi_Static_Netmask_ta,"0123456789.:");
  lv_obj_align_to(WiFi_Static_Netmask_ta,WiFi_Static_Netmask_lbl,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  lv_textarea_set_text(WiFi_Static_Netmask_ta,Static_Netmask.c_str());

  WiFi_Static_Gateway_lbl=lv_label_create(obj);
  lv_label_set_text(WiFi_Static_Gateway_lbl,"Gateway:");
  lv_obj_align_to(WiFi_Static_Gateway_lbl,WiFi_Static_Netmask_ta,LV_ALIGN_OUT_BOTTOM_LEFT,0,5);

  WiFi_Static_Gateway_ta=lv_textarea_create(obj);
  lv_textarea_set_one_line(WiFi_Static_Gateway_ta,true);
  lv_textarea_set_password_mode(WiFi_Static_Gateway_ta,false);
  lv_textarea_set_max_length(WiFi_Static_Gateway_ta,15);
  lv_textarea_set_accepted_chars(WiFi_Static_Gateway_ta,"0123456789.:");
  lv_obj_align_to(WiFi_Static_Gateway_ta,WiFi_Static_Gateway_lbl,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  lv_textarea_set_text(WiFi_Static_Gateway_ta,Static_Gateway.c_str());

  WiFi_Back3_btn=lv_btn_create(obj);
  label=lv_label_create(WiFi_Back3_btn);
  lv_label_set_text(label,"Back");
  lv_obj_align_to(WiFi_Back3_btn,WiFi_Static_Gateway_ta,LV_ALIGN_OUT_RIGHT_TOP,10,0);
  WiFi_Back3_btn->user_data=(void *)callback;

  WiFi_Next3_btn=lv_btn_create(obj);
  label=lv_label_create(WiFi_Next3_btn);
  lv_label_set_text(label,"Next");
  lv_obj_align_to(WiFi_Next3_btn,WiFi_Back3_btn,LV_ALIGN_OUT_RIGHT_TOP,0,0);
  WiFi_Next3_btn->user_data=(void *)callback;

  WiFi_kb = lv_keyboard_create(obj);
  lv_obj_add_flag(WiFi_kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_mode(WiFi_kb,LV_KEYBOARD_MODE_NUMBER);
  lv_obj_set_height(WiFi_kb,lv_obj_get_content_height(obj)-(lv_obj_get_y(WiFi_Static_Gateway_ta)+lv_obj_get_height(WiFi_Static_Gateway_ta)));
  

  switch(IP_Method)
  {
    case WiFi_IP_Method_DHCP:
    lv_obj_add_flag(WiFi_Static_IP_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_IP_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Netmask_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Netmask_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Gateway_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(WiFi_Static_Gateway_ta, LV_OBJ_FLAG_HIDDEN);
    break;
    case WiFi_IP_Method_Static:
    lv_obj_clear_flag(WiFi_Static_IP_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_Static_IP_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_Static_Netmask_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_Static_Netmask_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_Static_Gateway_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(WiFi_Static_Gateway_ta, LV_OBJ_FLAG_HIDDEN);
    break;
  }

  lv_obj_add_event_cb(WiFi_Static_IP_ta,WiFi_Static_All_ta_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Static_Netmask_ta,WiFi_Static_All_ta_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Static_Gateway_ta,WiFi_Static_All_ta_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_IP_Method_dd,WiFi_IP_Method_dd_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Back3_btn,WiFi_Back3_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(WiFi_Next3_btn,WiFi_Next3_btn_event_cb,LV_EVENT_ALL,NULL);

  DisplayWiFiConfig3_CheckValid();
}

void DisplayWiFiConfig4(lv_obj_t *obj,WiFiConfig4Callback callback)
{
  lv_obj_t * label;

  lv_obj_t * WiFi_wait_sp=lv_spinner_create(obj,800,240);
  lv_obj_align_to(WiFi_wait_sp,obj,LV_ALIGN_CENTER,0,0);
  lv_refr_now(NULL);

  if(WiFi_client.connected())
  {
    WiFi_client.stop();
  }

  WiFi.disconnect();
  WiFi.end();
  switch(WiFi_ConfigTemp_IP_Method)
  {
    case WiFi_IP_Method_DHCP:
    WiFi.config("");
    break;
    case WiFi_IP_Method_Static:
    WiFi.config(WiFi_ConfigTemp_Static_IP.c_str(),WiFi_ConfigTemp_Static_Netmask.c_str(),WiFi_ConfigTemp_Static_Gateway.c_str());
    break;
  }
  WiFi_status=WiFi.begin(WiFi_ConfigTemp_SSID.c_str(),WiFi_ConfigTemp_Pass.c_str(),WiFi_ConfigTemp_encryptionType);
  lv_refr_now(NULL);
  if(WiFi_status!=WL_CONNECTED)
  {
    if(callback)
    {
      callback(true);
    }
  }
  else
  {
    if(callback)
    {
      setNtpTime(timeServer);
      DataLog("Synchronized NTP time.");    
      callback(false);
    }
  }
}

void NTP_Server_ta_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_FOCUSED:
    case LV_EVENT_PRESSED:
    case LV_EVENT_CLICKED:
    case LV_EVENT_LONG_PRESSED:
    case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_keyboard_set_textarea(NTP_Server_kb,NTP_Server_ta);
    lv_obj_clear_flag(NTP_Server_kb, LV_OBJ_FLAG_HIDDEN);
    return;
    case LV_EVENT_DEFOCUSED:
    lv_keyboard_set_textarea(NTP_Server_kb,NULL);
    lv_obj_add_flag(NTP_Server_kb, LV_OBJ_FLAG_HIDDEN);
    return;
  }
}

void NTPServer_Apply_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    String Temp=lv_textarea_get_text(NTP_Server_ta);
    if(!Temp.isEmpty() && Temp!="")
    {
      saveSetting("timeServer",Temp);
      timeServer=Temp;
    }
    else
    {
      saveSetting("timeServer","pool.ntp.org");
      timeServer="pool.ntp.org";
      lv_textarea_set_text(NTP_Server_ta,"pool.ntp.org");
    }
    DataLog("Time server set to: "+timeServer);
    lv_obj_clean(WiFi_Config_Display_obj);
    lv_obj_t * WiFi_wait_sp=lv_spinner_create(WiFi_Config_Display_obj,800,240);
    lv_obj_align_to(WiFi_wait_sp,WiFi_Config_Display_obj,LV_ALIGN_CENTER,0,0);
    lv_refr_now(NULL);
    setNtpTime(timeServer);
    DataLog("Synchronized NTP time.");    
    DisplayMainStatusPanel(WiFi_Config_Display_obj);
    return;
  }
}

void NTPServer_Cancel_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    DisplayMainStatusPanel(WiFi_Config_Display_obj);
    return;
  }
}

void DisplayNTPServerConfig(lv_obj_t *obj)
{
  lv_obj_t *label;

  label=lv_label_create(obj);
  lv_label_set_text(label,"NTP Server Address:");
  NTP_Server_ta=lv_textarea_create(obj);
  lv_textarea_set_one_line(NTP_Server_ta,true);
  lv_textarea_set_password_mode(NTP_Server_ta,false);
  lv_textarea_set_accepted_chars(NTP_Server_ta,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.");
  lv_textarea_set_text(NTP_Server_ta,timeServer.c_str());
  lv_obj_align_to(NTP_Server_ta,label,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
  lv_obj_set_width(NTP_Server_ta,700);

  NTP_Server_kb=lv_keyboard_create(obj);
  lv_obj_add_flag(NTP_Server_kb, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t * NTPServer_Apply_btn=lv_btn_create(obj);
  label=lv_label_create(NTPServer_Apply_btn);
  lv_label_set_text(label,"Apply");
  lv_obj_align_to(NTPServer_Apply_btn,NTP_Server_ta,LV_ALIGN_OUT_BOTTOM_RIGHT,0,5);

  lv_obj_t * NTPServer_Cancel_btn=lv_btn_create(obj);
  label=lv_label_create(NTPServer_Cancel_btn);
  lv_label_set_text(label,"Cancel");
  lv_obj_align_to(NTPServer_Cancel_btn,NTPServer_Apply_btn,LV_ALIGN_OUT_LEFT_BOTTOM,-5,0);

  lv_obj_add_event_cb(NTP_Server_ta, (lv_event_cb_t)NTP_Server_ta_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(NTPServer_Cancel_btn,(lv_event_cb_t)NTPServer_Cancel_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(NTPServer_Apply_btn, (lv_event_cb_t)NTPServer_Apply_btn_event_cb,LV_EVENT_ALL,NULL);
}

void MainStatus_WiFiConfig_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    lv_obj_clean(WiFi_Config_Display_obj);
    DisplayWiFiConfig1(WiFi_Config_Display_obj,WiFi_SSID,WiFi_config1_callback);
    return;
  }

}

void MainStatus_WiFiStatus_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    lv_obj_clean(WiFi_Config_Display_obj);
    DisplayWiFiStatusPanel(WiFi_Config_Display_obj);
    return;
  }
}

void MainStatus_NTPServer_btn_event_cb(lv_event_t * event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    lv_obj_clean(WiFi_Config_Display_obj);
    DisplayNTPServerConfig(WiFi_Config_Display_obj);
    return;
  }
}

void WiFiStatus_Back_btn_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_CLICKED:
    DisplayMainStatusPanel(WiFi_Config_Display_obj);
    return;
  }
}

void DisplayWiFiStatusPanel(lv_obj_t *obj)
{
  lv_obj_clean(obj);

  lv_obj_t *label;

  wl_status_t status=(wl_status_t)(WiFi.status());
  String StatusString;
  switch(status)
  {
    #define narf(x) case x: StatusString=#x;break;
    narf(WL_NO_MODULE)
    narf(WL_IDLE_STATUS)
    narf(WL_NO_SSID_AVAIL)
    narf(WL_SCAN_COMPLETED)
    narf(WL_CONNECTED)
    narf(WL_CONNECT_FAILED)
    narf(WL_CONNECTION_LOST)
    narf(WL_DISCONNECTED)
    narf(WL_AP_LISTENING)
    narf(WL_AP_CONNECTED)
    narf(WL_AP_FAILED)
    default:
    StatusString="Unknown";
    break;
    #undef narf
  }

  lv_obj_t *ip_info_table=lv_table_create(WiFi_Config_Display_obj);
  #define narf(t,r) lv_table_set_cell_value(ip_info_table,r,0,t);
  narf("Status",0);
  narf("Mac Address",1);
  narf("SSID",2);
  narf("IP",3);
  narf("Netmask",4);
  narf("Gateway",5);
  #undef narf
  #define narf(t,r) lv_table_set_cell_value(ip_info_table,r,1,t);
  narf(StatusString.c_str(),0);
  String macAddress=WiFi.macAddress();
  if(!macAddress.isEmpty())
  {
    macAddress.toUpperCase();
    narf(macAddress.c_str(),1);
  }
  String SSID_and_Enc=WiFi.SSID();
  wl_enc_type encryptionType=(wl_enc_type)(WiFi.encryptionType());
  String Enc=EncryptionTypeDisplayStrings[EncryptionTypeToCBIMap[encryptionType]];
  SSID_and_Enc+=" ["+Enc+"]";
  narf(SSID_and_Enc.c_str(),2)
  narf(WiFi.localIP().toString().c_str(),3);
  narf(WiFi.subnetMask().toString().c_str(),4);
  narf(WiFi.gatewayIP().toString().c_str(),5);
  #undef narf
  lv_table_set_col_width(ip_info_table,0,150);
  lv_table_set_col_width(ip_info_table,1,300);
  lv_obj_t *WiFiStatus_Back_btn=lv_btn_create(obj);
  label=lv_label_create(WiFiStatus_Back_btn);
  lv_label_set_text(label,"Back");
  lv_obj_align_to(WiFiStatus_Back_btn,ip_info_table,LV_ALIGN_OUT_RIGHT_BOTTOM,10,0);

  lv_obj_add_event_cb(WiFiStatus_Back_btn,WiFiStatus_Back_btn_event_cb,LV_EVENT_ALL,NULL);
}

void DateTimeDisplay_lbl_event_cb(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target=lv_event_get_target(event);//button matrix
  void *user_data=target->user_data;
  switch(code)
  {
    case LV_EVENT_RELEASED:
    case LV_EVENT_DELETE:
    DateTimeDisplay_lbl=NULL;
    return;
  }
}

void DisplayMainStatusPanel(lv_obj_t *obj)
{
  lv_obj_clean(obj);

  lv_obj_t *label;

  lv_obj_t *lv_WiFiConfig_btn=lv_btn_create(obj);
  label=lv_label_create(lv_WiFiConfig_btn);
  lv_label_set_text(label,"WiFi Config");

  lv_obj_t *lv_WiFiStatus_btn=lv_btn_create(obj);
  label=lv_label_create(lv_WiFiStatus_btn);
  lv_label_set_text(label,"WiFi Status");
  lv_obj_align_to(lv_WiFiStatus_btn,lv_WiFiConfig_btn,LV_ALIGN_OUT_RIGHT_MID,0,0);

  lv_obj_t *lv_NTPServer_btn=lv_btn_create(obj);
  label=lv_label_create(lv_NTPServer_btn);
  lv_label_set_text(label,"NTP Server");
  lv_obj_align_to(lv_NTPServer_btn,lv_WiFiStatus_btn,LV_ALIGN_OUT_RIGHT_MID,0,0);

  LV_IMG_DECLARE(logo);
  lv_obj_t *lv_Logo_img=lv_img_create(obj);
  lv_img_set_src(lv_Logo_img, &logo);
  lv_obj_align(lv_Logo_img, LV_ALIGN_RIGHT_MID, -20, 0);

  label=lv_label_create(obj);
  lv_obj_align(label,LV_ALIGN_BOTTOM_LEFT,0,0);
  String Temp=getLocaltime();
  lv_label_set_text(label,Temp.c_str());
  DateTimeDisplay_lbl=label;

  lv_obj_add_event_cb(lv_WiFiConfig_btn,MainStatus_WiFiConfig_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(lv_WiFiStatus_btn,MainStatus_WiFiStatus_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(lv_NTPServer_btn,MainStatus_NTPServer_btn_event_cb,LV_EVENT_ALL,NULL);
  lv_obj_add_event_cb(DateTimeDisplay_lbl,DateTimeDisplay_lbl_event_cb,LV_EVENT_ALL,NULL);
}

void setup() {
  String Temp=loadSetting("WiFi_encryptionType");
  WiFi_encryptionType=(wl_enc_type)(Temp.toInt());

  WiFi_SSID=loadSetting("WiFi_SSID");

  WiFi_Pass=loadSetting("WiFi_Pass");

  Temp=loadSetting("WiFi_keyIndex");
  WiFi_keyIndex=(int)(Temp.toInt());

  Temp=loadSetting("WiFi_IP_Method");
  WiFi_IP_Method=(WiFi_IP_Method_enum_t)(Temp.toInt());

  WiFi_Static_IP=loadSetting("WiFi_Static_IP");

  WiFi_Static_Netmask=loadSetting("WiFi_Static_Netmask");

  WiFi_Static_Gateway=loadSetting("WiFi_Static_Gateway");

  Temp=loadSetting("timeServer");
  if(!Temp.isEmpty() && Temp!="")
  {
    timeServer=Temp;
  }
  
  Serial.begin(115200);
  byte mac[]={0x00,0x00,0x00,0x00,0x00,0x00};
  void *narf=WiFi.macAddress(mac);

  Display.begin();
  TouchDetector.begin();
  DataLogStart();

  /* Create a container with grid 1x1 */
  static lv_coord_t col_dsc[] = {755, 755, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {436, 436, LV_GRID_TEMPLATE_LAST};
  lv_obj_t * cont = lv_obj_create(lv_scr_act());
  lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
  lv_obj_set_size(cont, Display.width(), Display.height());
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x03989e), LV_PART_MAIN);
  lv_obj_center(cont);

  lv_obj_t * label;
  lv_obj_t * obj;

  obj = lv_obj_create(cont);
  lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 0, 1,
                        LV_GRID_ALIGN_STRETCH, 0, 1);

  WiFi_Config_Display_obj=obj;

  if(WiFi_SSID!="" && !WiFi_SSID.isEmpty())
  {
    lv_obj_t * WiFi_wait_sp=lv_spinner_create(obj,800,240);
    lv_obj_align_to(WiFi_wait_sp,obj,LV_ALIGN_CENTER,0,0);
    lv_refr_now(NULL);

    switch(WiFi_IP_Method)
    {
      case WiFi_IP_Method_DHCP:
      WiFi.config("");
      break;
      case WiFi_IP_Method_Static:
      WiFi.config(WiFi_Static_IP.c_str(),WiFi_Static_Netmask.c_str(),WiFi_Static_Gateway.c_str());
      break;
    }
    WiFi_status=WiFi.begin(WiFi_SSID.c_str(),WiFi_Pass.c_str(),WiFi_encryptionType);
    /*if(WiFi_status!=WL_CONNECTED)
    {
      if(callback)
      {
        callback(true);
      }
    }
    else
    {
      if(callback)
      {
        callback(false);
      }
    }*/
    if(WiFi_status==WL_CONNECTED)
    {
      setNtpTime(timeServer);
    }
    lv_obj_del(WiFi_wait_sp);
  }

  DataLog("Start");

  DisplayMainStatusPanel(obj);
}

void loop() { 
  /* Feed LVGL engine */
  lv_timer_handler();
  if(millis()>printNow)
  {
    if(DateTimeDisplay_lbl!=NULL)
    {
      String Temp=getLocaltime();
      lv_label_set_text(DateTimeDisplay_lbl,Temp.c_str());
    }
    printNow = millis() + printInterval;
  }
}

static String Debug_EventCodeToString(lv_event_code_t code)
{
  String Res="Unknown";
  #define narf(x) case x: Res=#x; break;
  switch(code)
  {
    narf(LV_EVENT_PRESSED)
    narf(LV_EVENT_PRESSING)
    narf(LV_EVENT_PRESS_LOST)
    narf(LV_EVENT_SHORT_CLICKED)
    narf(LV_EVENT_LONG_PRESSED)
    narf(LV_EVENT_LONG_PRESSED_REPEAT)
    narf(LV_EVENT_CLICKED)
    narf(LV_EVENT_RELEASED)
    narf(LV_EVENT_SCROLL_BEGIN)
    narf(LV_EVENT_SCROLL_END)
    narf(LV_EVENT_SCROLL)
    narf(LV_EVENT_GESTURE)
    narf(LV_EVENT_KEY)
    narf(LV_EVENT_FOCUSED)
    narf(LV_EVENT_DEFOCUSED)
    narf(LV_EVENT_LEAVE)
    narf(LV_EVENT_HIT_TEST)
    narf(LV_EVENT_COVER_CHECK)
    narf(LV_EVENT_REFR_EXT_DRAW_SIZE)
    narf(LV_EVENT_DRAW_MAIN_BEGIN)
    narf(LV_EVENT_DRAW_MAIN)
    narf(LV_EVENT_DRAW_MAIN_END)
    narf(LV_EVENT_DRAW_POST_BEGIN)
    narf(LV_EVENT_DRAW_POST)
    narf(LV_EVENT_DRAW_POST_END)
    narf(LV_EVENT_DRAW_PART_BEGIN)
    narf(LV_EVENT_DRAW_PART_END)
    narf(LV_EVENT_VALUE_CHANGED)
    narf(LV_EVENT_INSERT)
    narf(LV_EVENT_REFRESH)
    narf(LV_EVENT_READY)
    narf(LV_EVENT_CANCEL)
    narf(LV_EVENT_DELETE)
    narf(LV_EVENT_CHILD_CHANGED)
    narf(LV_EVENT_CHILD_CREATED)
    narf(LV_EVENT_CHILD_DELETED)
    narf(LV_EVENT_SCREEN_UNLOAD_START)
    narf(LV_EVENT_SCREEN_LOAD_START)
    narf(LV_EVENT_SCREEN_LOADED)
    narf(LV_EVENT_SCREEN_UNLOADED)
    narf(LV_EVENT_SIZE_CHANGED)
    narf(LV_EVENT_STYLE_CHANGED)
    narf(LV_EVENT_LAYOUT_CHANGED)
    narf(LV_EVENT_GET_SELF_SIZE)
  }
  #undef narf
  return Res;
}
