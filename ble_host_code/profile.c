#include "profile.h"
#include "update_thread.h"


profiles profile = {
    .data_handle=8,
    .non_cyclic_data_handle = 11,
    .key_id_handle=15,
    .xml_data_handle=17,
    .xml_control_handle=19,
    .meta_data_handle=21
};

//Temporarily hardcoded has to be decided dynamically.
char *portname = "/dev/ttyACM0";
char *keys_file =  "../configuration_files/keys.txt";
char *xml_file = "../configuration_files/xml.xml";
char *non_cyclic_data_file = "../configuration_files/xml.xml";
char *setup_file = "../configuration_files/setup.txt";