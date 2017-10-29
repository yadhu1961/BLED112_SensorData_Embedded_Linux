/* System headers */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <errno.h>
#include "encrypt.h"
#include "cmd_def.h"
#include "update_thread.h"
#include "apitypes.h"
#include "uart.h"
#include "profile.h"

enum actions action = action_none;

struct xml_files
{
    char data[5*1024];
    uint16 data_length;
}xml_file;

int number_transactions = 0;
int file_index = 0,current_transaction_count=0;

void change_state(states new_state)
{
        if(DEBUG) printf("DEBUG: State changed: %s --> %s\n", state_names[state], state_names[new_state]);
        state = new_state;
}
/**
 * Relevant event response handlers moved from stubs.c to here
 * STARTS
 *
 **/

void ble_rsp_system_get_info(const struct ble_msg_system_get_info_rsp_t *msg)
{
    if(DEBUG)printf("Build: %u, protocol_version: %u, hardware: ", msg->build, msg->protocol_version);

    switch (msg->hw)
    {
        case 0x01: if(DEBUG)printf("BLE112"); break;
        case 0x02: if(DEBUG)printf("BLED112"); break;
        default: if(DEBUG)printf("Unknown"); // I have no idea why BLED112 is responding with unknown hw version is it a bug in BLED112?
    }
    if(DEBUG)printf("\n");
}

void ble_evt_connection_feature_ind(const struct ble_msg_connection_feature_ind_evt_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    if(DEBUG)printf("\nconnection handle %x len %x data ",msg->connection,msg->features.len);
#if 0
    //This code is useful for better understanding of the ble stack. Have a look at it later.
    uint8 len = 0;
    for(len = 0; len < msg->features.len;len++)
        printf("%x",(char *)msg->features.data[len]);
    printf("\n");
#endif
}

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    //Here we are manually disconnecting the bluetooth which shouldnt be the case.

    if(msg->flags & connection_connected)
    {
        if(DEBUG)printf("Status connected\n");
        change_state(state_connected);

        // if(msg->flags & 0x03)
        // {
        //     if(DEBUG)printf("Now sending encrypt command\n");
        //     ble_cmd_sm_encrypt_start(msg->connection,1); //Trying to encrypt the connection.
        //     uart_rx();
        // }
    }
    else
    {
        printf("Not connected\n");
        ble_cmd_gap_set_mode(gap_general_discoverable,gap_undirected_connectable); //Again making it discoverable
    }
}


void ble_evt_connection_disconnected(const struct ble_msg_connection_disconnected_evt_t *msg)
{
    if(DEBUG) printf("%s\n",__func__ );
    invalidate_values(3);
    change_state(state_disconnected);
    ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
    //uart_rx();
    change_state(state_advertising);
}

//This event gets called when an attribute is written by the remote device.
void ble_evt_attributes_value(const struct ble_msg_attributes_value_evt_t *msg)
{

    if(DEBUG)printf("%s\n",__func__);

    //unsigned char value[2];
    //memcpy(value,msg->value.data,msg->value.len);
    //Here sending key id for verification
    //unsigned char *key_index = ()(msg->value.data);
    //printf("Value of key index %x\n",*key_index);
    if(verify_key_id(&(msg->value)))
            change_state(state_authenticated);
    else
    {
        printf("%s\n", "Key index invalid hence disconnecting");
        invalidate_values(3);
        //Resetting the values which determine the xml transfer states.
        file_index = 0;
        current_transaction_count = 0;
        //Disconnect the active connection if one is established
        ble_cmd_connection_disconnect(msg->connection);
        //uart_rx();
        ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
        //uart_rx();
        change_state(state_advertising);
    }
}

/* Instead of reading file when the master asks for data, I will fill the local buffer(This buffer resides in RAM this method is
    is  chosen since memory is not limiting factor here) with the file data in the beginning itself to save time from the
    expensive file read operation and I will keep that file data in a local buffer of max size 5 KB, This increases the throughput
    since Its not required to read the data */

int fill_file_buffer(char *xml_file_path)
{
    FILE *fp;
    char ch;
    fp = fopen(xml_file_path,"r");

    if(NULL == fp)
    {
        printf("opening of xml file failed,XML transfer feature won't work\n");
        printf("error %d opening %s: %s\n", errno, xml_file_path, strerror (errno) );
        return -1;
    }

    xml_file.data_length = fread(xml_file.data,1,1024*5,fp);
     if(DEBUG)printf("number of bytes read from file %d\n",xml_file.data_length);
    for(int count = 0 ; count< xml_file.data_length;count++)
        if(DEBUG)printf("%x", xml_file.data[count]);
    printf("\n");
    //In each user read from BLED112 I can transfer 600 bytes.
    number_transactions = xml_file.data_length/600;
    if(xml_file.data_length % 600 > 0 )
        number_transactions+=1;
    fclose(fp);
    return 0;
}

void ble_evt_attributes_user_read_request(const struct ble_msg_attributes_user_read_request_evt_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    uint8 null_value[] = {0,0};

    //Following part is to make sure that if the client reads this characteristic for the second time
    //Notifiable characteristic is set to zero.
    if(file_index == 0)
        ble_cmd_attributes_write(profile.control_service.xml_control.handle,0,2,null_value);

    if(DEBUG)printf("number of number_transactions %d \n",number_transactions );
    if(DEBUG)printf("current_transaction_count %d\n",current_transaction_count);
    if(DEBUG)printf("connection handle %d\n",msg->connection );
    if(DEBUG)printf("attribute handle %d\n",msg->handle );
    if(DEBUG)printf("offset of attribute %d\n",msg->offset );
    if(DEBUG)printf("max size %d\n",msg->maxsize );
    if(DEBUG)printf("value of file_index %d\n",file_index );
    ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,xml_file.data+file_index);
    /*here I need to count number of bytes sent. In the 594th trasaction although I send 22 bytes.
    The BLED112 accepts only six bytes, because the characteristic size is 600 bytes.(22-6) bytes
    ignored there increment the file index only by 6, not by 22.
    */
    if(msg->offset == 594)
    {
        file_index = file_index+6;
        current_transaction_count++;
    }
    else
        file_index = file_index + 22;

    /*Here when the curreny transactions equal to required number of
    Transactions then write the notifiable attribute with length file.
    So that the master gets notified that complete file is transferred.
    TO DO. I have to make this attribute notifiable.
    */
    if(current_transaction_count == number_transactions)
    {
        if(DEBUG)printf("%s\n","file transfer finished" );
        ble_cmd_attributes_write(profile.control_service.xml_control.handle,0,2,&xml_file.data_length);
        file_index = 0; //Again Initializing the file index to zero. Since the file transfer is complete.
        current_transaction_count = 0;
    }
}

/**
 * Relevant event response handlers moved from stubs.c to here
 * ENDS
 *
 * Handles of different attributes are present in the firmware.(taken from the attributes.txt file)
 *
 **/

void print_help()
{
    printf("\n\n--------------------------------------------------------------------------------------------------------------------------------------------\n\n");
    printf("B and R Automation Host application\n\n");
    printf("Run binary in the following format\n\n");
    printf("sudo ./ble_host_app.out portname keys_file_path sensor_permissions_file_path xml_file_path attributes_file_path \n\n");
    printf("--------------------------------------------------------------------------------------------------------------------------------------------\n\n\n");
}


int main(int argc, char *argv[] )
{
    //Temporarily hardcoded has to be decided dynamically.
    char *portname;// = "/dev/ttyACM1";
    char *keys_file;// =  "/home/yadhu/.br/keys.txt";
    char *xml_file;// = "/home/yadhu/.br/gatt.xml";
    char *attribute_file;// = "/home/yadhu/.br/attributes.txt";
    char *sensor_permission_file;// = "/home/yadhu/.br/sensor_permissions.txt";

    if(argc < 5)
     {
         print_help();
         return -1;
     }

    portname = argv[1];
    keys_file = argv[2];
    sensor_permission_file = argv[3];
    xml_file = argv[4];
    attribute_file = argv[5];

    if(DEBUG)printf("port name %s and keys_file %s\n",portname,keys_file);
    if(fill_file_buffer(xml_file) < 0)
    {
        printf("Unable to open xml file, XML transfer functionality won't work\n");
    }

    if(!keys_init(keys_file))
    {
        printf("Key intialization failed\n");
        exit(EXIT_FAILURE);
    }

    if(!profile_init(attribute_file,sensor_permission_file))
       printf("profile initialization failed will use hard attribute handles and sensor permissions\n");

    uart_open(portname);

     if (serial_handle < 0)
     {
         fprintf (stderr, "error %d opening %s: %s\n", errno, portname, strerror (errno));
         return -1;
     }

    bglib_output=uart_tx;

    ble_cmd_system_get_info(); //I don't know why, hardware is shown as unknown, Need to Figure out later
    //uart_rx();
    //Disconnect the active connection if one is established
    ble_cmd_connection_disconnect(0);
    //uart_rx();

    //Stop advertising
    ble_cmd_gap_set_mode(gap_non_discoverable, gap_non_connectable);
    //uart_rx();

    //stop previous operation
    ble_cmd_gap_end_procedure();
    //uart_rx();

    //Start Advertising
    ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
    //uart_rx();

    //Update the state machine
    change_state(state_advertising);

    //create update thread
    update_thread_init();

    //Message loop
    while(1);
    //{
        // if(uart_rx())
        // {
        //     printf("Error reading message\n");
        //     break;
        // }
    //}
    return 0;
}
