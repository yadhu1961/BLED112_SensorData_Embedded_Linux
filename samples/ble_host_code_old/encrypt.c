
#include "encrypt.h"

/* AES-128 bit EBC Encryption */
AES_KEY AESKey;

/* Head of keys linked list */
struct keys *head_node = NULL;
struct keys *current_node = NULL;
struct keys *authenticated_key = NULL;
unsigned keys_count=0;

static void print_linked_list();

/* AES_encrypt encrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void encrypt(unsigned char *key,unsigned char *text,unsigned char *cipher)
{
    if(DEBUG)printf("Key used for Encryption ");
    for(int i = 0; i<16;i++)
        if(DEBUG)printf("%c",*(key++));
    if(DEBUG)printf("\n");
    AES_set_encrypt_key((const unsigned char *) key, 128, &AESKey);
    AES_encrypt(text, cipher, &AESKey);
}

/* AES_decrypt decrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void decrypt(unsigned char *key,unsigned char *text,unsigned char *cipher)
{
    AES_set_decrypt_key((const unsigned char *) key, 128, &AESKey);
    AES_decrypt(cipher, text, &AESKey);
}

unsigned char checksum(unsigned char *ptr, size_t sz)
{
    unsigned char chk = 0;
    while (sz-- != 0)
        chk -= *ptr++;
    return chk;
}

bool keys_init(char *keys_file)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(keys_file, "r");
    if (fp == NULL) {
        printf("Failed to open the key file\n");
        return false;
    }
    while ((read = getline(&line, &len, fp)) != -1)
    {
        //Here 0x23 is ascii equivalent of hash(#) symbol that indicates that line is an comment in key file.
        //Here 0x0A means the line feed character, If the first character is line feed then its assumed that line is
        //an empty line.0x20 means space character.
        if(line[0] == 0x23 || line[0] == 0x0A || line[0] == 0x20)
            continue;

        if(head_node == NULL)
        {
            head_node  = (struct keys *)malloc(sizeof(struct keys));
            current_node = head_node;
            memcpy(head_node->key,line,16);
            current_node->permission = atoi(&line[18]);
            current_node->next =NULL;
        }
        else
        {
            current_node->next = (struct keys *)malloc(sizeof(struct keys));
            current_node = current_node->next;
            memcpy(current_node->key,line,16);
            current_node->permission = atoi(&line[18]);
            current_node->next =NULL;
        }
        current_node->key_id = keys_count;
        keys_count++;
    }
    current_node->next=NULL;
    if(DEBUG)print_linked_list();
    return true;
}

static void print_linked_list()
{
    printf("%s\n", __func__);

    if(head_node == NULL)
    {
        printf("Linked list is empty returning\n");
        return;
    }
    current_node = head_node;

    while(current_node != NULL)
    {
        for(int i = 0;i<16;i++)
            printf("%c",current_node->key[i] );
        printf(" key id %d ",current_node->key_id);
        printf("permission level %d\n",current_node->permission);
        current_node = current_node->next;
    }
    return;
}


bool verify_key_id(const uint8array *value)
{
    if(DEBUG) printf("%s value received from remote device with len %d\n",__func__,value->len);
    if(DEBUG)
    {
        for(int i = value->len-1;i>=0;i--)
            printf("%x ",value->data[i] );
        printf("\n");
    }
    unsigned short *int_value  = (unsigned short *)value->data;
    if(DEBUG)printf("char array converted to int %hu\n",*int_value);

    if(head_node == NULL)
    {
        if(DEBUG)printf("key list is empty returning\n");
        return false;
    }
    current_node = head_node;
    while(current_node != NULL)
    {
        if(*int_value == current_node->key_id)
        {
            if(DEBUG)printf("received key id is valid\n");
            //Here I am checking of the memory was allocated previously if so, I am releasing it.
            if(NULL == authenticated_key)
                authenticated_key = (struct keys *)malloc(sizeof(struct keys));
            memcpy(authenticated_key->key,current_node->key,16);
            authenticated_key->key_id = current_node->key_id;
            authenticated_key->permission = current_node->permission;
            return true;
        }
        current_node = current_node->next;
    }
    if(DEBUG)printf("key index not found in the linked list hence returning false\n");
    return false;
}

#ifdef test_checksum

int main(int argc, char* argv[])
{
    unsigned char x[] = {0x01,0x02,0x03,0x04};
    unsigned char y = checksum (x, 5);
    printf ("Checksum is 0x%02x\n", y);
    x[5] = y;
    y = checksum (x, 6);
    printf ("Checksum test is 0x%02x\n", y);
    return 0;
}

#endif


#ifdef test

//This code is testing this module for correct behaviour. Enable when its necessary to test the code

int main( )
{
    /* Input data to encrypt */
    unsigned char aes_input[]={0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x07,0x08,0x09,0x00,0x01,0x02,0x03,0x04,0x05};

    /* Buffers for Encryption and Decryption */
    unsigned char enc_out[sizeof(aes_input)];
    unsigned char dec_out[sizeof(aes_input)];

    printf("value of input array %d\n",sizeof(aes_input));

    printf("value of input array %d\n",sizeof(enc_out));

    printf("value of input array %d\n",sizeof(dec_out));

    /* AES-128 bit CBC Decryption */
    //memset(iv, 0x00, AES_BLOCK_SIZE); // don't forget to set iv vector again, else you can't decrypt data properly
    // AES_set_decrypt_key((const unsigned char *) aes_key, 128, &AESKey);
    // AES_decrypt(enc_out, dec_out,&AESKey);
    encrypt(aes_key,aes_input,enc_out);

    decrypt(aes_key,dec_out,enc_out) ;


    printf("value of input array %d\n",sizeof(aes_input));

    printf("value of input array %d\n",sizeof(enc_out));

    printf("value of input array %d\n",sizeof(dec_out));

    /* Printing and Verifying */
    print_data("\n Original ",aes_input, sizeof(aes_input)); // you can not print data as a string, because after Encryption its not ASCII

    print_data("\n Encrypted",enc_out, sizeof(enc_out));

    print_data("\n Decrypted",dec_out, sizeof(dec_out));

    return 0;
}

void print_data(const char *tittle, const void* data, int len)
{
    printf("%s : ",tittle);
    const unsigned char * p = (const unsigned char*)data;
    int i = 0;

    for (; i<7; ++i)
    	printf("%02X ", *p++);

    printf("\n");
}

#endif
