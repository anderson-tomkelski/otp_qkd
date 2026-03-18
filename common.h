#ifndef COMMON_H
#define COMMON_H

#define MAX_KEY_SIZE 4096
#define MAX_DATA_SIZE 4096
#define KEY_ID_SIZE 64

typedef struct {
    char key_id[KEY_ID_SIZE];
    unsigned char key[MAX_KEY_SIZE];
    int key_len;
} otp_key_t;

#endif