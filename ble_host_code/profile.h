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
    uint8 data_handle;
    uint8 non_cyclic_data_handle;
    uint8 key_id_handle;
    uint8 xml_data_handle;
    uint8 xml_control_handle;
    uint8 meta_data_handle;
} profiles;

extern profiles profile;

//Temporarily hardcoded has to be decided dynamically.
extern char *portname;// = "/dev/ttyACM0";
extern char *keys_file;// =  "../configuration_files/keys.txt";
extern char *xml_file;// = "../configuration_files/xml.xml";
extern char *non_cyclic_data_file;// = "../configuration_files/xml.xml";
extern char *setup_file;// = "../configuration_files/setup.txt";

#endif //end for encrypt_INCLUDED