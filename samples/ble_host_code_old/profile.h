#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cmd_def.h"

#ifndef profile_INCLUDED
#define profile_INCLUDED

typedef struct 
{
    char char_uuid[32];
    uint8_t handle;
}characteristics;

typedef struct 
{
    char service_uuid[32];
    characteristics x,y,z;
    uint8_t permission_level;
}gyro_sensors;

typedef struct 
{
    char service_uuid[32];
    characteristics position;
    uint8_t permission_level;
}position_sensors;

typedef struct
{
    char service_uuid[32];
    characteristics key_id,xml_data,xml_control;
}control_services;

typedef struct 
{
    gyro_sensors gyro_sensor;
    position_sensors position_sensor;
    control_services control_service; 
} profiles;

bool profile_init(char *,char *);

extern profiles profile;

#endif //end for encrypt_INCLUDED