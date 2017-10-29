#include "update_thread.h"
#include "cmd_def.h"

uint8_t invalid_values[DATA_ATTRIBUTE_SIZE];
uint8_t null_values[10];
pthread_t update_values_thread;
char *data_file = NULL;
uint8_t data[DATA_ATTRIBUTE_SIZE] = {0};
uint8_t data_output[DATA_ATTRIBUTE_SIZE] = {0};
int data_update_period = 1000; //Every second;
int key_update_period = 10000; //Every 10 seconds;
int key_update_counter = 10;

int update_thread_init()
{
    key_update_counter = key_update_period/data_update_period;
    memset(invalid_values,0xff,DATA_ATTRIBUTE_SIZE);
    memset(null_values,0x00,10);
    if(pthread_create(&update_values_thread,NULL,update_attribute_thread,NULL))
    {
        printf("%s\n","Failed to create update thread exit" );
        return -1;
    }
}

static bool read_sensor_data()
{
    int bytes_read = 0;
    FILE *fp = fopen(data_file, "r");
    if(fp == NULL)
        return false;
    bytes_read = fread(data, sizeof(unsigned char), DATA_ATTRIBUTE_SIZE, fp);
    if(DEBUG)printf("number of bytes read from file %d\n",bytes_read);
    if(DEBUG)print_data("sensor data file contents",data,bytes_read);
    fclose(fp);
    return true;
}

static void write_long_attributes(uint8 handle,int length,unsigned char *data_local)
{
    if(length <= MAX_ATTRIBUTE_PAYLOAD )
    {
        if(DEBUG)printf("Data length less than BG Protocol maximum packet length\n");
        if(DEBUG)("Hence data can transferred as a single packet");
            ble_cmd_attributes_write(handle,0,length,data_local);
    }
    else
    {
        if(DEBUG)printf("Data length greater than 60 bytes hence needs multiple BG ");
        if(DEBUG)printf("transactions needed\n");
        //Here calculate how many transactions are needed.
        //One extra transaction is needed for remainder bytes
        int transactions_required = length / MAX_ATTRIBUTE_PAYLOAD;//transactions_required += 1;
        int total_bytes_transferred = 0,tx_count = 0,remaining_bytes = length % MAX_ATTRIBUTE_PAYLOAD;
        for(tx_count = 0;tx_count<transactions_required;tx_count++)
            ble_cmd_attributes_write(handle,tx_count*MAX_ATTRIBUTE_PAYLOAD,MAX_ATTRIBUTE_PAYLOAD,data_local+tx_count*MAX_ATTRIBUTE_PAYLOAD);
        total_bytes_transferred = tx_count*MAX_ATTRIBUTE_PAYLOAD;
        ble_cmd_attributes_write(handle,total_bytes_transferred,remaining_bytes,data_local+total_bytes_transferred);
    }
}

void *update_attribute_thread(void *argptr)
{
    static int freq_counter = 0;
    if(DEBUG)printf("%s\n",__func__);
    //This variable is for overwriting the sensor values with junk values as soon as smart phone disconnects.
    while(1)
    {
        //This is temporary, Has to fill valid data once things are ready.
        memset(data,0,DATA_ATTRIBUTE_SIZE);
        usleep(1000*data_update_period); //Updates the attributes for every second, If needed can be updated with in less time.
        freq_counter = freq_counter + 1;
        if(freq_counter == key_update_counter)
        {
            if(DEBUG)printf("Reloading the keys\n");
            if(!keys_update(keys_file))
            {
                printf("Key update failed will use the old keys\n");
            }
            freq_counter = 0;
        }
        if(state == state_authenticated)
        {
            if(!read_sensor_data())
                printf("Failed to read data from sensor data file,Attribute will contain zero's\n");
            if(DEBUG)print_data("data buffer after reading sensor values",data,241);
            data[DATA_ATTRIBUTE_SIZE-1] = checksum(data,DATA_ATTRIBUTE_SIZE-1);
            if(DEBUG)printf("Hello there, hear me, I am writing attributes to database\n");
            encrypt(authenticated_key->key,data,data);
            if(DEBUG)print_data("Encrypted value of data",data,DATA_ATTRIBUTE_SIZE);
            write_long_attributes(profile.data_handle,DATA_ATTRIBUTE_SIZE,data);//here 8 is the handle to the characteristic
        }
        else{
            if(DEBUG)printf("%s\n","Skipping update, State is not authenticated" );
        }
    }
}

void print_data(const char *tittle, const void* data, int len)
{
    printf("%s : ",tittle);
    const unsigned char * p = (const unsigned char*)data;
    int i = 0;

    for (; i<len; ++i)
        printf("%02X ", *p++);
    printf("\n\n");
}


void invalidate_values(uint8_t flag)
{
    if(DEBUG)printf("Invalidating the values flag %d\n",flag);

    switch(flag)
    {
        case 1:
            write_long_attributes(profile.data_handle,DATA_ATTRIBUTE_SIZE,invalid_values);//Here 8 is the handle to the sensor data characteristic
            break;
        default:
            write_long_attributes(profile.data_handle,DATA_ATTRIBUTE_SIZE,invalid_values);
            ble_cmd_attributes_write(profile.xml_control_handle,0,2,null_values);
    }

}
