#ifndef KEY_FETCH_H
#define KEY_FETCH_H

#include "common.h"

int get_key_sender(otp_key_t *key, int size_bytes);
int get_key_receiver(const char *key_id, otp_key_t *key);

#endif