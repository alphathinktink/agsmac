
/*
  agsmac by Leonard W. Bogard

  created 11/13/2024

  use version 4.1.5 of the [Arduino Mbed OS Giga Boards] library.
  use version 8.3.11 of the [lvgl] library.
*/

#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

#include "lvgl.h"

#include "KVStore.h"
#include "kvstore_global_api.h"
#include "mbed.h"

//#include <SPI.h>
#include <WiFi.h>

Arduino_H7_Video          Display(800, 480, GigaDisplayShield); /* Arduino_H7_Video Display(1024, 768, USBCVideo); */
Arduino_GigaDisplayTouch  TouchDetector;

WiFiClient WiFi_client;

typedef void (*WiFiConfig3Callback)(bool IsBack,wl_enc_type encryptionType,const String &SSID,const String &Pass,int keyIndex);
typedef void (*WiFiConfig2Callback)(bool IsBack,wl_enc_type encryptionType,const String &SSID,const String &Pass,int keyIndex);
typedef void (*WiFiConfig1Callback)(bool IsCancel,const String &SSID,wl_enc_type encryptionType);

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

struct WiFi_scan_list_parameters
{
  String SSID;
  wl_enc_type encryptionType;
};
const char * EncryptionTypeDisplayStrings[]={"WEP","WPA/TKIP","WPA2/CCMP","WPA3/GCMP","None","Unknown","Auto"};
const wl_enc_type EncryptionTypeDisplayMap[]={ENC_TYPE_WEP,ENC_TYPE_WPA,ENC_TYPE_WPA2,ENC_TYPE_WPA3,ENC_TYPE_NONE,ENC_TYPE_UNKNOWN,ENC_TYPE_AUTO};
const int EncryptionTypeToCBIMap[]={5,5,1,5,2,0,3,4,6,5};

lv_obj_t * WiFi_Config_Display_obj=NULL;
lv_obj_t * WiFi_scan_list=NULL;
lv_obj_t * WiFi_Rescan_btn=NULL;
lv_obj_t * WiFi_SSID_ta=NULL;
lv_obj_t * WiFi_Pass_ta=NULL;
lv_obj_t * WiFi_kb=NULL;
lv_obj_t * WiFi_SSID_label;
lv_obj_t * WiFi_Next1_btn=NULL;
lv_obj_t * WiFi_Back1_btn=NULL;
lv_obj_t * WiFi_EncType_dd=NULL;
lv_obj_t * WiFi_WepKeyIndex_dd=NULL;
lv_obj_t * WiFi_WepKeyIndex_lbl=NULL;
String WiFi_SSID="";
wl_enc_type WiFi_encryptionType=ENC_TYPE_AUTO;
String WiFi_Pass="";
int WiFi_keyIndex=1;
volatile int WiFi_status=WL_IDLE_STATUS;


void WiFi_config3_callback(bool IsBack,wl_enc_type encryptionType,const String &SSID,const String &Pass,int keyIndex)
{
  lv_obj_clean(WiFi_Config_Display_obj);
  if(IsBack)
  {
    const char * btn_txts[]={"Abort","Start Over",NULL};
    lv_obj_t * mbox=lv_msgbox_create(NULL, "WiFi Connection", "Failed", btn_txts, false);
    //lv_obj_align_to(mbox,WiFi_Config_Display_obj,LV_ALIGN_CENTER,0,0);
    lv_obj_center(mbox);
  }
  else
  {
    const char * btn_txts[]={"OK","Abort","Start Over",NULL};
    lv_obj_t * mbox=lv_msgbox_create(NULL, "WiFi Connection", "Success", btn_txts, false);
    //lv_obj_align_to(mbox,WiFi_Config_Display_obj,LV_ALIGN_CENTER,0,0);
    lv_obj_center(mbox);
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
      String SSID="";
      if(WiFi_SSID_ta!=NULL)
      {
        SSID=lv_textarea_get_text(WiFi_SSID_ta);
      }
      String Pass="";
      if(WiFi_Pass_ta!=NULL)
      {
        Pass=lv_textarea_get_text(WiFi_Pass_ta);
      }
      int keyIndex=1;
      if(WiFi_WepKeyIndex_dd!=NULL)
      {
        keyIndex=lv_dropdown_get_selected(WiFi_WepKeyIndex_dd)+1;
      }
      wl_enc_type encryptionType=ENC_TYPE_AUTO;
      if(WiFi_EncType_dd!=NULL)
      {
        encryptionType=EncryptionTypeDisplayMap[lv_dropdown_get_selected(WiFi_EncType_dd)];
      }
      callback(false,encryptionType,SSID.c_str(),Pass.c_str(),keyIndex);
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
      String SSID="";
      if(WiFi_SSID_ta!=NULL)
      {
        SSID=lv_textarea_get_text(WiFi_SSID_ta);
      }
      String Pass="";
      if(WiFi_Pass_ta!=NULL)
      {
        Pass=lv_textarea_get_text(WiFi_Pass_ta);
      }
      callback(true,ENC_TYPE_AUTO,SSID.c_str(),Pass.c_str(),0);
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
      callback(true,"",ENC_TYPE_AUTO);
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
      callback(false,SSID,encryptionType);
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

void WiFi_config1_callback(bool IsCancel,const String &SSID, wl_enc_type encryptionType)
{
  if(IsCancel)return;
  lv_obj_clean(WiFi_Config_Display_obj);
  bool DontNeedSSID=true;
  if(SSID.isEmpty() || strcmp(SSID.c_str(),"")==0)
  {
    DontNeedSSID=false;
  }
  DisplayWiFiConfig2(WiFi_Config_Display_obj,encryptionType,DontNeedSSID,SSID,"",1,WiFi_config2_callback);
}

void WiFi_config2_callback(bool IsBack,wl_enc_type encryptionType,const String &SSID,const String &Pass,int keyIndex)
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
    DisplayWiFiConfig3(WiFi_Config_Display_obj,encryptionType,SSID,Pass,keyIndex,WiFi_config3_callback);
    return;
  }
}

void DisplayWiFiConfig3(lv_obj_t *obj,wl_enc_type encryptionType,const String &SSID,const String &Pass,int KeyIndex,WiFiConfig3Callback callback)
{
  lv_obj_t * label;

  lv_obj_t * WiFi_wait_sp=lv_spinner_create(obj,800,240);
  lv_obj_align_to(WiFi_wait_sp,obj,LV_ALIGN_CENTER,0,0);

  if(WiFi_client.connected())
  {
    WiFi_client.stop();
  }

  WiFi.disconnect();
  WiFi_status=WiFi.begin(SSID.c_str(),Pass.c_str(),encryptionType);
  if(WiFi_status!=WL_CONNECTED)
  {
    if(callback)
    {
      callback(true,(wl_enc_type)0,"","",1);
    }
  }
  else
  {
    if(callback)
    {
      callback(false,encryptionType,SSID,Pass,KeyIndex);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Display.begin();
  TouchDetector.begin();

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

//WiFi_SSID
  //saveSetting("WiFi_SSID","KGM_Offices");
  //WiFi_SSID=loadSetting("WiFi_SSID");
  //erial.println(WiFi_SSID);

  DisplayWiFiConfig1(obj,WiFi_SSID,WiFi_config1_callback);


}

void loop() { 
  /* Feed LVGL engine */
  lv_timer_handler();
}

