#pragma once
#include <Arduino.h>
#include <map>
#include <string>
#include "Version.h"

#ifndef DATATOPIC
    #define DATATOPIC "%s/sensors/%s/state"
#endif

#define DISCOVERYTOPIC "homeassistant/sensor/%s/%s-%s/config"

#define STR(A) #A
#define JSON_START "{"
#define JSON_END  "}"
#define STATE_TOPIC "\"stat_t\": \"" DATATOPIC "\","
#define UNIQUE_ID "\"uniq_id\": \"%s-%s\","
#define ORIGIN "\"o\":{\"name\":\"Efergy Bridge\",\"sw\":\"" VERSION "\",\"url\":\"" GIHUBURL "\"},"
#define DEVICE "\"dev\": { \"name\": \"%s\", \"mdl\": \"%s\",\"ids\":[\"%s\"] },"


struct discoveryDevice {
    const char *device_type;
    const char *object_suffix;
    const char *configTemplate;
};

#define DISCOVERYMESSAGE(buffer,id,field,model,protocol)  \
     char buffer[strlen(hostname)+(strlen(id)*2)+strlen(discoveryConfigMap[field].object_suffix)+strlen(discoveryConfigMap[field].configTemplate)]; \
     sprintf(buffer,discoveryConfigMap[field].configTemplate,hostname,id,id,discoveryConfigMap[field].object_suffix,model,protocol,id); 

/* Original Source https://github.com/merbanan/rtl_433/blob/master/examples/rtl_433_mqtt_hass.py */
std::map<String, discoveryDevice>discoveryConfigMap = {
    {"temperature_C", {
        .device_type="sensor",
        .object_suffix="T", 
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE
            "\"dev_cla\": \"temperature\","
            "\"name\": \"Temperature\","
            "\"unit_of_meas\": \"°C\","
            "\"val_tpl\": \"{{ value_json.temperature_C|float|round(1) }}\","
            "\"stat_cla\": \"measurement\"" 
            JSON_END                  
    }}, 

    {"battery_ok", {
        .device_type= "sensor",
        .object_suffix= "B",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE 
            "\"dev_cla\": \"battery\","
            "\"name\": \"Battery\","
            "\"unit_of_meas\": \"%\","
            "\"val_tpl\": \"{{ float(value_json.battery_ok) * 99 + 1 }}\","
            "\"stat_cla\": \"measurement\","
            "\"ent_cat\": \"diagnostic\""
            JSON_END                  
    }},

    {"rssi", {
        .device_type= "sensor",
        .object_suffix= "rssi",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE 
            "\"dev_cla\": \"signal_strength\","
            "\"unit_of_meas\": \"dB\","
            "\"val_tpl\": \"{{ value_json.rssi|float|round(2) }}\","
            "\"stat_cla\": \"measurement\","
            "\"ent_cat\": \"diagnostic\""
            JSON_END                  
    }},

    {"power_W", {
        .device_type= "sensor",
        .object_suffix= "watts",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE
            "\"dev_cla\": \"power\","
            "\"name\": \"Power\","
            "\"unit_of_meas\": \"W\","
            "\"val_tpl\": \"{{ value_json.power_W|float }}\","
            "\"stat_cla\": \"measurement\""
            JSON_END                  
    }},

    {"energy_kWh", {
        .device_type= "sensor",
        .object_suffix= "kwh",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE
            "\"dev_cla\": \"power\","
            "\"name\": \"Energy\","
            "\"unit_of_meas\": \"kWh\","
            "\"val_tpl\": \"{{ value_json.energy_kWh|float }}\","
            "\"stat_cla\": \"measurement\""
            JSON_END                  
    }},

    {"current_A", {
        .device_type= "sensor",
        .object_suffix= "A",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE
            "\"dev_cla\": \"current\","
            "\"name\": \"Current\","
            "\"unit_of_meas\": \"A\","
            "\"val_tpl\": \"{{ value_json.current_A|float }}\","
            "\"stat_cla\": \"measurement\""
            JSON_END                  
    }},
  
    {"voltage_V", {
        .device_type= "sensor",
        .object_suffix= "V",
        .configTemplate=
            JSON_START STATE_TOPIC UNIQUE_ID ORIGIN DEVICE
            "\"dev_cla\": \"power\","
            "\"name\": \"Voltage\","
            "\"unit_of_meas\": \"V\","
            "\"val_tpl\": \"{{ value_json.voltage_V|float }}\","
            "\"stat_cla\": \"measurement\""
            JSON_END                  
    }}
                                                    }; 
