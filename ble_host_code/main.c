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
#include "ezxml/ezxml.h"
//This header file is added to get the file size of the file.
#include <sys/stat.h>


enum actions action = action_none;

struct xml_files
{
    unsigned char data[5*1024];
    uint16 data_length;
}xml_file_data;

struct non_cyclic_data
{
    unsigned char data[5*1024];
    uint16 data_length;
}non_cyclic_file_data = {{0},0};

typedef struct
{
    uint8 total_transcount;
    uint8 current_trans_count;
    uint8 payload[504];
}packets;

//Allocating continuos buffer to store xml_packets and non cyclic data packets in a continous manner;
packets xml_packet[10],non_cyclic_data_packet[5];

//This is required when the state is not authenticated but user tries to read the data.
char dummy_data[600] = {0};
//This variable is needed to store the value smartphone writes the value to
//characteristic and that value needs to stored in the data file.
//This buffer acts as temporary buffer.
char data_read[100] = {0};
static int offset_sensor_data = 0;
int data_file_size  = 0;

int number_transactions = 0,non_cyclic_number_transactions = 0;
int file_index = 0,current_transaction_count=0,non_cyclic_file_index = 0,non_cyclic_currrent_transcount = 0;

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

//This code is added for alarm handling, Every time someone connect
//Alarm gets configured if he doesn't write a valid key id within 5 seconds
//Dongle will disconnect from the smart phone.
void timer_call_back()
{
    //Here i am is the state still connected after 5 seconds
    if(state == state_connected) {
        if(DEBUG)printf("Valid key id is not written after %d seconds\nso Disconnecting\n",INTERVAL);
        //Disconnect the active connection if one is established
        ble_cmd_connection_disconnect(0);
        //uart_rx();
        ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
        //uart_rx();
        change_state(state_advertising);
    }
}


//This configures a alarm which will be checked after 5 seconds to verify whether
//Valid connection established with a smartphone.
 int configure_alarm(int interval)
 {
    struct itimerval it_val;
    it_val.it_value.tv_sec = interval;
    it_val.it_value.tv_usec = 0;
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = 0;


    if (signal(SIGALRM, timer_call_back) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        return -1;
    }

    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
        perror("error calling setitimer()");
        return -1;
    }

    // struct sigaction sact;
    // volatile double count;
    // time_t t;

    // sigemptyset(&sact.sa_mask);
    // sact.sa_flags = 0;
    // sact.sa_handler = catcher;
    // sigaction(SIGALRM, &sact, NULL);

    // alarm(5); /* timer will pop in five seconds */

 }

void ble_evt_connection_status(const struct ble_msg_connection_status_evt_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    //Here we are manually disconnecting the bluetooth which shouldnt be the case.

    if(msg->flags & connection_connected)
    {
        if(DEBUG)printf("Status connected\n");
        change_state(state_connected);
        configure_alarm(INTERVAL);
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
    change_state(state_advertising);
    file_index = 0;
    current_transaction_count = 0;
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
    if(msg->handle == profile.key_id_handle) //When handle equal to 12 key is written.
    {
            if(verify_key_id(&(msg->value)))
                change_state(state_authenticated);
            else
            {
                printf("%s\n", "Key index invalid hence disconnecting");
                invalidate_values(3);
                //Resetting the values which determine the file transfer states.
                file_index = 0;
                current_transaction_count = 0;
                non_cyclic_file_index = 0;
                non_cyclic_currrent_transcount = 0;
                //Disconnect the active connection if one is established
                ble_cmd_connection_disconnect(msg->connection);
                //uart_rx();
                ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
                //uart_rx();
                change_state(state_advertising);
            }
    }
    else if(state == state_authenticated || state == state_finding_attributes)
    {
        if(DEBUG)printf("Number of bytes written %d\n",msg->value.len);
        if(state == state_authenticated) //I have to read only for the first time.
            ble_cmd_attributes_read(msg->handle,0);
        state = state_finding_attributes;
    } else
        printf("Unless authentication is done, one cannot write to sensor data file\n");
}

int save_data_file()
{
    if(DEBUG)printf("%s\n",__func__);
    sleep(1);//This is to make sure that file is closed by update thread.
    int bytes_written = 0;
    FILE *fp = fopen(data_file, "w");
    if(fp == NULL)
        return false;
    bytes_written = fwrite(data_read,1,data_file_size,fp);
    if(DEBUG)printf("number of bytes written to the file %d\n",bytes_written);
    if(DEBUG)print_data("new sensor data file contents",data_read,bytes_written);
    fclose(fp);
    //Resetting data_local buffer to zero.
    memset(data_read,0,data_file_size);
    //Once the data is written, Move to transmitting state
    state = state_authenticated;
    return true;
}

//When we read data from the local attribute database this call back gets called
void ble_rsp_attributes_read(const struct ble_msg_attributes_read_rsp_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    if(msg->handle == profile.data_handle)
    {
        memcpy(data_read+offset_sensor_data,msg->value.data,msg->value.len);
        offset_sensor_data = offset_sensor_data + msg->value.len;
        if(offset_sensor_data < data_file_size) {
            ble_cmd_attributes_read(msg->handle,offset_sensor_data);
        }
        else
            save_data_file();
    } else if(msg->handle == profile.key_id_handle)
    {
        if(verify_key_id(&(msg->value)))
            change_state(state_authenticated);
        else
        {
            printf("%s\n", "Key index invalid hence disconnecting");
            invalidate_values(3);
            //Resetting the values which determine the xml transfer states.
            file_index = 0;
            current_transaction_count = 0;
            non_cyclic_file_index = 0;
            current_transaction_count = 0;
            //Disconnect the active connection if one is established
            ble_cmd_connection_disconnect(0);
            //uart_rx();
            ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
            //uart_rx();
            change_state(state_advertising);
        }
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
    char command[100];
    int return_value;
    pid_t process_pid;
    char zipfile_name[50];
    /*
    To create a zip file I am running a linux command which compresses the xml file so that
    file size becomes smaller
    */

    process_pid = getpid();
    sprintf(zipfile_name,"/tmp/zipfile_%d.zip",process_pid);
    if(DEBUG) printf("zipfile name %s\n",zipfile_name);
    sprintf(command,"zip -j %s %s",zipfile_name,xml_file_path);
    if(DEBUG)printf("command to be executed %s\n", command);
    return_value = system(command);
    if(DEBUG)printf("command returned %d\n",return_value);
    if(return_value != 0)
    {
        printf("compressing xml failed xml transfer won't work\n");
        return -1;
    }

    fp = fopen(zipfile_name,"r");

    if(NULL == fp)
    {
        printf("opening of xml file failed,XML transfer feature won't work\n");
        printf("error %d opening %s: %s\n", errno, xml_file_path, strerror (errno) );
        return -1;
    }

    xml_file_data.data_length = fread(xml_file_data.data,1,1024*5,fp);
    if(DEBUG)printf("number of bytes read from file %d\n",xml_file_data.data_length);

    number_transactions = xml_file_data.data_length/504;
    if(xml_file_data.data_length % 504 > 0 )
        number_transactions+=1;
    if(DEBUG)printf("Number of xml_packets needed %d\n",number_transactions);

    //Filling the xml packets;
    for(int i = 0;i<number_transactions;i++) {
        xml_packet[i].total_transcount = number_transactions;
        xml_packet[i].current_trans_count = i;
        memcpy(xml_packet[i].payload,xml_file_data.data+i*504,504);
    }

    memcpy(xml_file_data.data,xml_packet,506*number_transactions);

    for(int count = 0 ; count< xml_file_data.data_length;count++)
        if(DEBUG)printf("%x", xml_file_data.data[count]);
    printf("\n");
    //In each user read from BLED112 I can transfer 600 bytes.
    fclose(fp);
    return 0;
}


int non_cyclic_file_buffer(char *non_cyclic_file_path)
{
    FILE *fp;

    fp = fopen(non_cyclic_file_path,"r");

    if(NULL == fp)
    {
        printf("opening of non_cyclic_file failed,non_cyclic_data transfer feature won't work\n");
        printf("error %d opening %s: %s\n", errno, non_cyclic_file_path, strerror (errno) );
        return -1;
    }

    non_cyclic_file_data.data_length = fread(non_cyclic_file_data.data,1,1024*5,fp);
    if(DEBUG)printf("number of bytes read from %s %d\n",non_cyclic_file_path,non_cyclic_file_data.data_length);

    non_cyclic_number_transactions = non_cyclic_file_data.data_length/504;
    if(non_cyclic_file_data.data_length % 504 > 0 )
        non_cyclic_number_transactions+=1;
    if(DEBUG)printf("Number of non_cyclic_packets needed %d\n",non_cyclic_number_transactions);

    //Filling the non_cyclic packets;
    for(int i = 0;i<non_cyclic_number_transactions;i++) {
        non_cyclic_data_packet[i].total_transcount = non_cyclic_number_transactions;
        non_cyclic_data_packet[i].current_trans_count = i;
        memcpy(non_cyclic_data_packet[i].payload,non_cyclic_file_data.data+i*504,504);
    }

    memcpy(non_cyclic_file_data.data,non_cyclic_data_packet,506*non_cyclic_number_transactions);

    for(int count = 0 ; count< non_cyclic_file_data.data_length;count++)
        if(DEBUG)printf("%x", non_cyclic_file_data.data[count]);
    if(DEBUG)printf("\n");
    //In each user read from BLED112 I can transfer 600 bytes.
    fclose(fp);
    return 0;
}

void ble_evt_attributes_user_read_request(const struct ble_msg_attributes_user_read_request_evt_t *msg)
{
    if(DEBUG)printf("%s\n",__func__);
    if(DEBUG)printf("attribute handle %d\n",msg->handle);
    if(DEBUG)printf("xml Total transactions required %d \n",number_transactions );
    if(DEBUG)printf("xml current_transaction_count %d\n",current_transaction_count);
    if(DEBUG)printf("offset of attribute %d\n",msg->offset );
    if(DEBUG)printf("max size %d\n",msg->maxsize );
    if(DEBUG)printf("value of xml file_index %d\n",file_index );
    if(DEBUG)printf("non_cyclic_data Total transactions required %d \n",non_cyclic_number_transactions);
    if(DEBUG)printf("non_cyclic_data current_transaction_count %d\n",non_cyclic_currrent_transcount);
    if(DEBUG)printf("value of non_cyclic_data file_index %d\n",non_cyclic_file_index );

    /*here I need to count number of bytes sent. In the 594th trasaction although I send 22 bytes.
    The BLED112 accepts only six bytes, because the characteristic size is 600 bytes.(22-6) bytes
    ignored there increment the file index only by 6, not by 22.
    */
    if(msg->handle == profile.xml_data_handle)
    {
        if(msg->offset <= 484)
        {
            ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,xml_file_data.data+file_index);
            file_index = file_index+22;
            if(msg->offset == 484) {
                current_transaction_count++;
            }
        }
        else
            ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,dummy_data);

        /*Here when the curreny transactions equal to required number of
        Transactions then write the notifiable attribute with length file.
        So that the master gets notified that complete file is transferred.
        */
        if(current_transaction_count == number_transactions)
        {
            if(DEBUG)printf("File transfer finished Making file_index zero");
            //Again Initializing the file index to zero. Since the file transfer is complete.
            file_index = 0;
            current_transaction_count = 0;
        }
    }
    else
    {
        if(msg->offset <= 484)
        {
            ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,non_cyclic_file_data.data+non_cyclic_file_index);
            non_cyclic_file_index = non_cyclic_file_index+22;
            if(msg->offset == 484) {
                non_cyclic_currrent_transcount++;
            }
        }
        else
            ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,dummy_data);

        /*Here when the curreny transactions equal to required number of
        Transactions then write the notifiable attribute with length file.
        So that the master gets notified that complete file is transferred.
        */
        if(non_cyclic_currrent_transcount == non_cyclic_number_transactions)
        {
            if(DEBUG)printf("File transfer finished Making file_index zero");
            //Again Initializing the file index to zero. Since the file transfer is complete.
            non_cyclic_file_index = 0;
            non_cyclic_currrent_transcount = 0;
        }
    }
}


// void ble_evt_attributes_user_read_request(const struct ble_msg_attributes_user_read_request_evt_t *msg)
// {
//     if(DEBUG)printf("%s\n",__func__);
//     uint8 null_value[] = {0,0};

//     //Following part is to make sure that if the client reads this characteristic for the second time
//     //Notifiable characteristic is set to zero.
//     if(file_index == 0)
//         ble_cmd_attributes_write(profile.xml_control_handle,0,2,null_value);
//     //If the state is authenticated then only transfer the XML other
//     // if(state_authenticated == state)
//     // {
//         if(DEBUG)printf("number of number_transactions %d \n",number_transactions );
//         if(DEBUG)printf("current_transaction_count %d\n",current_transaction_count);
//         //if(DEBUG)printf("connection handle %d\n",msg->connection );
//         //if(DEBUG)printf("attribute handle %d\n",msg->handle );
//         if(DEBUG)printf("offset of attribute %d\n",msg->offset );
//         if(DEBUG)printf("max size %d\n",msg->maxsize );
//         if(DEBUG)printf("value of file_index %d\n",file_index );
//         ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,xml_file_data.data+file_index);
//         here I need to count number of bytes sent. In the 594th trasaction although I send 22 bytes.
//         The BLED112 accepts only six bytes, because the characteristic size is 600 bytes.(22-6) bytes
//         ignored there increment the file index only by 6, not by 22.

//         if(msg->offset == 220)
//         {
//             current_transaction_count++;
//         }
//         else
//             file_index = file_index + 22;

//         /*Here when the curreny transactions equal to required number of
//         Transactions then write the notifiable attribute with length file.
//         So that the master gets notified that complete file is transferred.
//         */
//         if(current_transaction_count == number_transactions)
//         {
//             if(DEBUG)printf("%s\n","file transfer finished" );
//             ble_cmd_attributes_write(profile.xml_control_handle,0,2,&xml_file_data.data_length);
//             if(DEBUG)printf("Making file index zero");
//             file_index = 0; //Again Initializing the file index to zero. Since the file transfer is complete.
//             current_transaction_count = 0;
//         }
//     // }
//     // else
//     //     ble_cmd_attributes_user_read_response(msg->connection,0,msg->maxsize,dummy_data);
// }



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
    printf("sudo ./ble_host_app.out portname keys_file_path xml_file_path sensor_data_file_path non_cyclic_data_file setup_file\n\n");



    printf("\nIf not specified will use hardcoded paths may not result in expected behavior\n");


    printf("--------------------------------------------------------------------------------------------------------------------------------------------\n\n\n");

}

int set_meta_data(char *xml_path,char *setup_file_path)
{
    ezxml_t team;
    char null_value[20] = {0};
    //Invalidating the value before writing the new value
    ble_cmd_attributes_write(profile.meta_data_handle,0,20,null_value);

    char *type,*variant,meta_data[50],*name,*line;
    FILE *fp = NULL;
    static int line_counter= 0;
    size_t len = 0;

    fp = fopen(setup_file_path,"r");
    if(NULL == fp)
    {
        printf("Failed to open setup file, proceeding to xml meta data\n");
    }
    else
    {
        while ((getline(&line, &len, fp) != -1) && line_counter <= 3)
        {
            //Here 0x23 is ascii equivalent of hash(#) symbol that indicates that line is an comment in key file.
            //Here 0x0A means the line feed character, If the first character is line feed then its assumed that line is
            //an empty line.0x20 means space character.
            if(line[0] == 0x23 || line[0] == 0x0A || line[0] == 0x20)
                continue;
            line_counter++;
            if(DEBUG)printf("data read from setup.txt %s\n", line);
            if(line_counter == 1)
                memcpy(meta_data,line,strlen(line)-1);
            else if(line_counter == 2)
                data_update_period = atoi(line);
            else if(line_counter == 3)
                key_update_period = atoi(line);
        }
        fclose(fp);
    }

    if(DEBUG)printf("Name of a device %s\n",meta_data );
    if(DEBUG)printf("Data_update_period %d\n",data_update_period);
    if(DEBUG)printf("key_file_update_period %d\n",key_update_period);

    ezxml_t f1 = ezxml_parse_file(xml_path);
    if(f1 == NULL) {
        printf("Unable to parse the xml so setting only name\n");
        ble_cmd_attributes_write(profile.meta_data_handle,0,strlen(meta_data),meta_data);
        return -1;
    }

    team = ezxml_child(f1, "Module");

    if(f1 == NULL) {
        printf("Unable to get the childso setting only name\n");
        ble_cmd_attributes_write(profile.meta_data_handle,0,strlen(meta_data),meta_data);
        return -2;
    }
    type = ezxml_attr(team,"Type");
    variant = ezxml_attr(team,"Variant");
    strcat(meta_data,";");
    strcat(meta_data,type);
    strcat(meta_data,";");
    strcat(meta_data,variant);
    ble_cmd_attributes_write(profile.meta_data_handle,0,strlen(meta_data),meta_data);
    if(DEBUG)printf("META_DATA %s\n",meta_data);
    ezxml_free(f1);
    return 0;
}

void sig_handler(int signo)
{
    printf("received SIGINT\n");
    if (signo == SIGINT) {
        printf("received SIGINT\n");
        printf("Gracefully exiting from process\n");

        //Disconnect the active connection if one is established
        ble_cmd_connection_disconnect(0);

        //Stop advertising
        ble_cmd_gap_set_mode(gap_non_discoverable, gap_non_connectable);

        //stop previous operation
        ble_cmd_gap_end_procedure();
        exit(0);
    }
}

// bool parse_setup_file(int char* setup_file_path)
// {
//     FILE *fp;

//     fp = fopen(setup_file_path,"r");

//     if(NULL == fp)
//     {
//         printf("opening of non_cyclic_file failed,non_cyclic_data transfer feature won't work\n");
//         printf("error %d opening %s: %s\n", errno, non_cyclic_file_path, strerror (errno) );
//         return -1;
//     }
// }

int get_file_size( char * file_name)
{
    struct stat st;
    stat(file_name, &st);
    return st.st_size;
}

int main(int argc, char *argv[] )
{
    //This variable defined in update_thread.h
    data_file = "../configuration_files/data.dat";

    if(argc < 4)
         print_help();
     else {
        portname = argv[1];
        keys_file = argv[2];
        xml_file = argv[3];
        //Here, I am initializing the sensor data file.
        data_file = argv[4];
        non_cyclic_data_file = argv[5];
        setup_file  = argv[6];
     }

     data_file_size = get_file_size(data_file);

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");

    if(DEBUG)printf("port name %s and keys_file %s\n",portname,keys_file);
    if(fill_file_buffer(xml_file) < 0)
    {
        printf("Unable to open xml file, XML transfer functionality won't work\n");
    }

    if(non_cyclic_file_buffer(non_cyclic_data_file) < 0)
    {
        printf("Unable to open %s file,its transfer functionality won't work\n",non_cyclic_data_file);
    }

    if(!keys_init(keys_file))
    {
        printf("Key intialization failed\n");
        exit(EXIT_FAILURE);
    }

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

    // uint8 sr_rsp[] = {0x06,gap_ad_type_localname_complete,0x59,0x41,0x44,0x48,0x55};

    // //set local name (scan response packet)
    // ble_cmd_gap_set_adv_data(1,7,sr_rsp);

    //uart_rx();

    //Start Advertising
    ble_cmd_gap_set_mode(gap_general_discoverable, gap_undirected_connectable);
    //uart_rx();

    //Update the state machine
    change_state(state_advertising);

    if(set_meta_data(xml_file,setup_file))
        printf("%s\n","Unable to set parse the xml correctly");

    //create update thread
    update_thread_init();

    // printf("xml file data len %d\n",xml_file_data.data_length);

    // //This block is to write first chunk of 255 to characteristic
    // printf("%s\n","Writing first chunk to xml_data characteristic");
    // for(int count = 0;count < 12;count++)
    //     ble_cmd_attributes_write(profile.xml_data_handle,count*22,22,xml_file_data.data+count*22);

    // ble_cmd_attributes_write(profile.xml_data_handle,242,13,xml_file_data.data+242);
    // current_transaction_count = 1;
    // ble_cmd_attributes_write(profile.xml_control_handle,0,1,&current_transaction_count);

    //Message loop
    //This infinite while loop was consuming lot of cpu resource hence
    //Using pause() which pauses the current thread until a signal is received.
    while(1)
    {
        sleep(10000);
    }
    //pause();
    //sigsuspend(SIGINT | SIGALRM | SIGKILL);


    return 0;
}
