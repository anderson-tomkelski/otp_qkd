#include "crypto.h"

void otp_encrypt(unsigned char *data, unsigned char *key, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= key[i];
    }
}