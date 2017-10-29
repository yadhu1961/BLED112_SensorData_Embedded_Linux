#include "uart.h"


int serial_handle;
pthread_t rx_thread;

void print_raw_response(struct ble_header *hdr, unsigned char *data);
void print_tx_packet(uint8,uint8 *,uint16,uint8 *);


int uart_open(char *port)
{
    struct termios options;
    int i;

    serial_handle = open(port, (O_RDWR | O_NOCTTY));
    if(serial_handle < 0)
    {
        printf("Failed to open the port serial handle %d\n",serial_handle);
            return -1;
    }
    else
        if(DEBUG)printf("Successfully opened the port serial_handle %d\n",serial_handle);

    tcgetattr(serial_handle, &options); //get the current options for the port...
    cfsetispeed(&options, B115200);     //set Baud rate to 115200
    cfsetospeed(&options, B115200);     //enable the receiver and set param...
    options.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS | HUPCL);
    options.c_cflag |= (CS8 | CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL | ECHOPRT | ECHOKE | IEXTEN);
    options.c_iflag &= ~(INPCK | IXON | IXOFF | IXANY | ICRNL);
    options.c_oflag &= ~(OPOST | ONLCR);
    for ( i = 0; i < sizeof(options.c_cc); i++ )
        options.c_cc[i] = _POSIX_VDISABLE;
    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 1;
    //set new opt for the port
    tcsetattr(serial_handle, TCSAFLUSH, &options);
    /*In addition to this Initialization of the serial port. I will start a to thread continuosly read the response from the serial port
    then calls the corresponding response handlers
    */
    if(pthread_create(&rx_thread,NULL,uart_rx,NULL))
    {
        printf("%s\n","Failed to create rx_thread exit" );
        return -1;
    }

    return 0;
}

void uart_close(){
        close(serial_handle);
}

void uart_tx(uint8 len1,uint8* data1,uint16 len2,uint8* data2)
{
    if(DEBUG)print_tx_packet(len1,data1,len2,data2);

    ssize_t written;
    while(len1)
    {
        written = write(serial_handle,data1,len1);
        if(!written)
            return;
        len1 -= written;
        data1 += len1;
     }

    while(len2)
    {
        written = write(serial_handle,data2,len2);
        if(!written)
        return;
        len2 -= written;
        data2 += len2;
     }

}

//Serial port read thread this continusly reads data from serial port
void *uart_rx( void *ptr)
{
     if(DEBUG)printf("%s\n","Serial read thread started" );
    const struct ble_msg *apimsg;
    struct ble_header apihdr;
    unsigned char data[256];//enough for BLE

    ssize_t rread;
    struct termios options;
    int header_size = 4;

    tcgetattr(serial_handle, &options);
    options.c_cc[VTIME] = timeout_ms/100;
    options.c_cc[VMIN] = 0;
    tcsetattr(serial_handle, TCSANOW, &options);

    while(1)
    {
        //printf("%s\n","reading data from the port" );
        rread = read(serial_handle, &apihdr, header_size);
        if(!rread)
        {
            //printf("%s\n","1 :read zero bytes" );
            continue;
        } else if(rread < 0)
        {
            //printf("%s\n","1: read negative bytes" );
            continue;
        }

        if(apihdr.lolen)
        {
            rread = read(serial_handle, data, apihdr.lolen);
            if(!rread)
                continue;
            else if(rread < 0)
                continue;
        }
        //printf("%s\n","Finding the header of the file" );
        apimsg=ble_get_msg_hdr(apihdr);

        if(!apimsg)
        {
            printf("ERROR: Message not found:%d:%d\n",(int)apihdr.cls,(int)apihdr.command);
            continue;
        }
        //Calling function callback based on the response received
        apimsg->handler(data);
        //printing the raw packet
        if(DEBUG)print_raw_response(&apihdr,data);
    }
}

void print_tx_packet(uint8 len1,uint8* data1,uint16 len2,uint8* data2)
{
    printf("SENT PACKET\n");

    for (int i = 0; i < len1; i++)
        printf("%02x ", ((unsigned char *)data1)[i]);

    for (int i = 0; i < len2; i++)
        printf("%02x ", data2[i]);

    printf("\n\n");
}


void print_raw_response(struct ble_header *hdr, unsigned char *data)
{
    printf("RECEIVED PACKET: \n");

    for (int i = 0; i < sizeof(*hdr); i++)
        printf("%02x ", ((unsigned char *)hdr)[i]);

    for (int i = 0; i < hdr->lolen; i++)
        printf("%02x ", data[i]);

    printf("\n\n");
}