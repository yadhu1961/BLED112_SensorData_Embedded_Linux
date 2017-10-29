#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <stdbool.h>
#include <stdint.h>
#include "apitypes.h"
#include "cmd_def.h"
#include "profile.h"

#ifndef encrypt_INCLUDED
#define encrypt_INCLUDED


/* AES-128 bit EBC Encryption */
extern AES_KEY AESKey;

typedef enum {
    admin,
    employee,
    permission_levels_count
} permission_level;


struct keys
{
    unsigned char key[16];
    unsigned  key_id;
    struct keys *next;
};

extern struct keys *authenticated_key;

extern unsigned keys_count;

/* AES_encrypt encrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void encrypt(unsigned char *key,unsigned char *text,unsigned char *cipher);


/* AES_decrypt decrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
void decrypt(unsigned char *key,unsigned char *text,unsigned char *cipher);

/* Function which calculates the 8-bit checksum and returns the value*/
unsigned char checksum (unsigned char *ptr, size_t sz);

/* This function reads the keys from the database stores in a linked list*/ 
bool keys_init(char *);

/*Used for reloading the keys file*/
bool keys_update(char *);

/* Verifies the key id sent from the smartphone to check with the database*/ 
bool verify_key_id(const uint8array *);

//This part of the code for module testing. To test the encryption module enable this code.
#ifdef test

/* AES key for Encryption and Decryption */
unsigned char aes_key[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

/* Print Encrypted and Decrypted data packets */
void print_data(const char *title, const void* data, int len);

#endif //end for #ifdef test

#endif //end for encrypt_INCLUDED