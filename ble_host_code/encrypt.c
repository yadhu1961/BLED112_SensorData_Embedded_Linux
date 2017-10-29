
#include "encrypt.h"

/* AES-128 bit EBC Encryption */
AES_KEY AESKey;

/* Head of keys linked list */
struct keys *head_node = NULL;
struct keys *current_node = NULL;
struct keys *authenticated_key = NULL;
unsigned int keys_count=0;

static void print_linked_list();
static void empty_linked_list();

/* AES_encrypt encrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void encrypt(unsigned char *key_old,unsigned char *text,unsigned char *cipher)
{
    if(DEBUG)printf("Key used for Encryption ");
    for(int i = 0; i<16;i++)
        if(DEBUG)printf("%x",*(key_old+i));
    if(DEBUG)printf("\n");
    AES_set_encrypt_key((const unsigned char *) key_old, 128, &AESKey);
    //Dividing the 240 bytes in to the chunks of 16 bytes,
    //Since AES can handle only a chunks of 16 bytes.
    for(int i=0;i<15;i++)
        AES_encrypt(text+i*16, cipher+i*16, &AESKey);
    //decrypt(key_old,text,cipher);
}

/* AES_decrypt decrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void decrypt(unsigned char *key,unsigned char *text,unsigned char *cipher)
{
    AES_set_decrypt_key((const unsigned char *) key, 128, &AESKey);
    //for(int i=0;i<15;i++)
    AES_decrypt(cipher, text, &AESKey);
    for(int i = 0; i<16;i++)
        if(DEBUG)printf("%x",text[i]);
}

unsigned char checksum(unsigned char *ptr, size_t sz)
{
    unsigned char chk = 0;
    while (sz-- != 0)
        chk -= *ptr++;
    return chk;
}

static void print_data(const char *tittle, const void* data, int len)
{
    printf("%s : ",tittle);
    const unsigned char * p = (const unsigned char*)data;
    int i = 0;

    for (; i<len; ++i)
        printf("%02X ", *p++);

    printf("\n");
}


unsigned char * ascii_to_hex(unsigned char *pos)
{
    size_t count = 0;
    unsigned char *val = (unsigned char *)malloc(17);
     /* WARNING: no sanitization or error-checking whatsoever */
    for(count = 0; count < 17; count++)
    {
        sscanf(pos, "%2hhx", &val[count]);
        pos += 2;
    }
    memcpy(pos,val,17);
    return val;
}

char *line = NULL;

bool keys_init(char *keys_file)
{
    FILE * fp;
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

        if(DEBUG)printf("data read from keys.txt %s\n", line);
        //Here I need to convert the read string key to hex
        unsigned char *ptr = ascii_to_hex(line);
        memcpy(line,ptr,17);
        //Freeing the memory allocated in func.
        free(ptr);

        if(head_node == NULL)
        {
            head_node  = (struct keys *)malloc(sizeof(struct keys));
            keys_count++;
            current_node = head_node;
            current_node->key_id = line[0];
            memcpy(head_node->key,line+1,16);
            current_node->next =NULL;
        }
        else
        {
            current_node->next = (struct keys *)malloc(sizeof(struct keys));
            keys_count++;
            current_node = current_node->next;
            current_node->key_id = line[0];
            memcpy(current_node->key,line+1,16);
            current_node->next =NULL;
        }
    }
    fclose(fp);
    if(DEBUG)print_linked_list();
    return true;
}

bool keys_update(char *keys_file)
{
    if(DEBUG)printf("Updating the keys\n");
    FILE * fp;
    size_t len = 0;
    ssize_t read;
    unsigned int tmp_key_count= 0;
    bool isListEmpty = false;

    fp = fopen(keys_file, "r");
    if (fp == NULL) {
        printf("Failed to open the key file\n");
        return false;
    }
    empty_linked_list();
    current_node = head_node;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        //Here 0x23 is ascii equivalent of hash(#) symbol that indicates that line is an comment in key file.
        //Here 0x0A means the line feed character, If the first character is line feed then its assumed that line is
        //an empty line.0x20 means space character.
        if(line[0] == 0x23 || line[0] == 0x0A || line[0] == 0x20)
            continue;
        tmp_key_count++;
        if(DEBUG)printf("data read from keys.txt %s\n", line);
        //Here I need to convert the read string key to hex
        unsigned char *ptr = ascii_to_hex(line);
        memcpy(line,ptr,17);
        //Freeing the memory allocated in func.
        free(ptr);

        if(head_node == NULL && read != 0)
        {
            printf("Keys list is empty try to build keys list now\n");
            isListEmpty = true;
            head_node  = (struct keys *)malloc(sizeof(struct keys));
            keys_count++;
            current_node = head_node;
            current_node->key_id = line[0];
            memcpy(head_node->key,line+1,16);
            current_node->next =NULL;
        }
        else
        {
            if(current_node == head_node && !isListEmpty)
            {
                current_node->key_id = line[0];
                memcpy(current_node->key,line+1,16);
                isListEmpty = true; //I am misusing variable just achieve the purpose :-) careful
            }
            else
            {
                if(current_node->next != NULL) {
                    current_node = current_node->next;
                    current_node->key_id = line[0];
                    memcpy(current_node->key,line+1,16);
                }
                else if(current_node->next == NULL && read != 0)
                {
                    printf("List finished but new keys are present hence adding a new entry to key list\n");
                    current_node->next = (struct keys *)malloc(sizeof(struct keys));
                    current_node = current_node->next;
                    keys_count++;
                    current_node->key_id = line[0];
                    memcpy(current_node->key,line+1,16);
                    current_node->next = NULL;
                    break;
                }
            }
        }
    }
    if(DEBUG)print_linked_list();
    fclose(fp);
    //Here I will read the key index from the file.
    //To reauthenticate with new keys.
    if(state == state_authenticated || authenticated_key != NULL)
        ble_cmd_attributes_read(profile.key_id_handle,0);
    return true;
}


// static void manual_authentication()
// {
//     if(DEBUG) printf("This function does manual_authentication after every time we read the keys file");
//     if(DEBUG)
//     {
//         for(int i = value->len-1;i>=0;i--)
//             printf("%d ",value->data[i] );
//         printf("\n");
//     }
//     unsigned char *int_value  = (unsigned char *)value->data;
//     if(DEBUG)printf("char array converted to int %d\n",*int_value);

//     if(head_node == NULL)
//     {
//         if(DEBUG)printf("key list is empty returning\n");
//         return false;
//     }
//     current_node = head_node;
//     while(current_node != NULL)
//     {
//         if(*int_value == current_node->key_id)
//         {
//             if(DEBUG)printf("received key id is valid\n");
//             //Here I am checking of the memory was allocated previously if so, I am releasing it.
//             if(NULL == authenticated_key)
//                 authenticated_key = (struct keys *)malloc(sizeof(struct keys));
//             memcpy(authenticated_key->key,current_node->key,16);
//             authenticated_key->key_id = current_node->key_id;
//             return true;
//         }
//         current_node = current_node->next;
//     }
//     if(DEBUG)printf("key index not found in the linked list hence returning false\n");
//     return false;
// }

static void empty_linked_list()
{
    current_node = head_node;
    while(current_node != NULL)
    {
        memset(current_node,0,17);
        current_node = current_node->next;
    }
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
            printf("%x",current_node->key[i] );
        printf(" key id %x \n",current_node->key_id);
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
            printf("%d ",value->data[i] );
        printf("\n");
    }
    unsigned char *int_value  = (unsigned char *)value->data;
    if(DEBUG)printf("char array converted to int %d\n",*int_value);

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
            //Here I am checking of the memory was allocated previously if not allocate.
            if(NULL == authenticated_key)
                authenticated_key = (struct keys *)malloc(sizeof(struct keys));
            memcpy(authenticated_key->key,current_node->key,16);
            authenticated_key->key_id = current_node->key_id;
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
