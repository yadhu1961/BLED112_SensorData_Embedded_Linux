#include "update_thread.h"
#include "cmd_def.h"

uint8_t invalid_values[17];
uint8_t null_values[10];
pthread_t update_values_thread;

int update_thread_init()
{
    memset(invalid_values,0xff,17);
    memset(null_values,0x00,10);
    if(pthread_create(&update_values_thread,NULL,update_attribute_thread,NULL))
    {
        printf("%s\n","Failed to create update thread exit" );
        return -1;
    }

}

void *update_attribute_thread(void *argptr)
{
    if(DEBUG)printf("%s\n",__func__);
    //This variable is for overwriting the sensor values with junk values as soon as smart phone disconnects.
    uint8_t x[17] = {0};
    uint8_t y[17] = {0};
    uint8_t z[17] = {0};
    uint8_t position[17] = {0};
    while(1)
    {
        //This is temporary, Has to fill valid data once things are ready.
        memset(x,0,17);
        memset(y,0,17);
        memset(z,0,17);
        memset(position,0,17);
        x[0] = 0x01;
        y[0] = 0x02;
        z[0] = 0x03;
        usleep(1000*1000); //Updates the attributes for every second, If needed can be updated with in less time.
        if(state == state_authenticated)
        {
            if(authenticated_key->permission >= profile.gyro_sensor.permission_level)
            {
                x[16] = checksum(x,16);
                y[16] = checksum(y,16);
                z[16] = checksum(z,16);
                if(DEBUG)printf("Hello there, hear me, I am writing attributes to database\n");
                encrypt(authenticated_key->key,x,x);
                if(DEBUG)print_data("Encrypted valueof x",x,16);
                ble_cmd_attributes_write(profile.gyro_sensor.x.handle,0,17,x);
                //uart_rx();
                encrypt(authenticated_key->key,y,y);
                if(DEBUG)print_data("Encrypted value of y",y,16);
                ble_cmd_attributes_write(profile.gyro_sensor.y.handle,0,17,y);
                //uart_rx();
                encrypt(authenticated_key->key,z,z);
                if(DEBUG)print_data("Encrypted value of z",z,16);
                ble_cmd_attributes_write(profile.gyro_sensor.z.handle,0,17,z);
                //uart_rx();
            }
            else
                invalidate_values(1);

            if(authenticated_key ->permission >= profile.position_sensor.permission_level)
            {
                position[0] = 0x04;
                position[16] = checksum(position,16);
                encrypt(authenticated_key->key,position,position);
                if(DEBUG)print_data("Encrypted value of position",position,17);
                ble_cmd_attributes_write(profile.position_sensor.position.handle,0,17,position);
                //uart_rx();
            }
            else
                invalidate_values(2);
        }
        else
            if(DEBUG)printf("%s\n","Skipping update, State is not authenticated" );
    }
}

void print_data(const char *tittle, const void* data, int len)
{
    printf("%s : ",tittle);
    const unsigned char * p = (const unsigned char*)data;
    int i = 0;

    for (; i<len; ++i)
        printf("%02X ", *p++);
    printf("\n");
}


void invalidate_values(uint8_t flag)
{
    if(DEBUG)printf("Invalidating the values flag %d\n",flag);

    switch(flag)
    {
        case 1:
            ble_cmd_attributes_write(profile.gyro_sensor.x.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.gyro_sensor.y.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.gyro_sensor.z.handle,0,17,invalid_values);
            break;
        case 2:
            ble_cmd_attributes_write(profile.position_sensor.position.handle,0,17,invalid_values);
            break;
        default:
            ble_cmd_attributes_write(profile.gyro_sensor.x.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.gyro_sensor.y.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.gyro_sensor.z.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.position_sensor.position.handle,0,17,invalid_values);
            ble_cmd_attributes_write(profile.control_service.xml_control.handle,0,2,null_values);
    }

}
