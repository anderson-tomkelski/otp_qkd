#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "key_fetch.h"

#define USE_API 1  // <-- 0 = mock


static const char *MOCK_KEY_ID = "TEST-123";
static const char *MOCK_KEY_BASE64 = "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=";

struct memory {
    char *response;
    size_t size;
};

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    char *ptr = realloc(mem->response, mem->size + total + 1);
    if (!ptr) return 0;

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, total);
    mem->size += total;
    mem->response[mem->size] = 0;

    return total;
}

int base64_decode(const char *input, unsigned char *output) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new_mem_buf((void*)input, -1);
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int len = BIO_read(bio, output, strlen(input));
    BIO_free_all(bio);

    return len;
}


int parse_mock(otp_key_t *key) {
    strcpy(key->key_id, MOCK_KEY_ID);
    key->key_len = base64_decode(MOCK_KEY_BASE64, key->key);

    printf("[MOCK] key_id=%s\n", key->key_id);

    return 0;
}


int fetch_from_api(const char *url, const char *cert, const char *key_file,
                   const char *cacert, const char *interface, otp_key_t *out_key) {

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    struct memory chunk = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
    curl_easy_setopt(curl, CURLOPT_SSLKEY, key_file);
    curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_INTERFACE, interface);

    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("curl error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

    printf("[API RESPONSE] %s\n", chunk.response);


    cJSON *json = cJSON_Parse(chunk.response);
    if (!json) {
        printf("JSON parse error\n");
        return -1;
    }

    // cJSON *key_id = cJSON_GetObjectItem(json, "key_ID");
    // cJSON *key_val = cJSON_GetObjectItem(json, "key");

    cJSON *keys = cJSON_GetObjectItem(json, "keys");

    if (!keys || !cJSON_IsArray(keys)) {
        printf("JSON missing 'keys' array\n");
        return -1;
    }

    cJSON *first = cJSON_GetArrayItem(keys, 0);

    if (!first) {
        printf("JSON 'keys' array vazio\n");
        return -1;
    }

    cJSON *key_id = cJSON_GetObjectItem(first, "key_ID");
    cJSON *key_val = cJSON_GetObjectItem(first, "key");

    if (!key_id || !key_val) {
        printf("JSON missing key fields\n");
        return -1;
    }

    // if (!key_id || !key_val) {
    //     printf("JSON missing fields\n");
    //     return -1;
    // }

    strcpy(out_key->key_id, key_id->valuestring);
    out_key->key_len = base64_decode(key_val->valuestring, out_key->key);

    cJSON_Delete(json);
    curl_easy_cleanup(curl);
    free(chunk.response);

    return 0;
}


int get_key_sender(otp_key_t *key, int size_bytes) {

#if USE_API
    char url[512];

    int bits = size_bytes * 8;

    snprintf(url, sizeof(url),
        "https://11.0.1.2:50051/api/v1/keys/bob_client1/enc_keys?size=%d",
        bits
    );

    return fetch_from_api(
        url,
        "alice_client1.crt",
        "alice_client1.key",
        "rootCA_auth.crt",
        "enx00e04c690502",
        key
    );
#else
    return parse_mock(key);
#endif
}


int get_key_receiver(const char *key_id, otp_key_t *key) {

#if USE_API
    char url[512];

    snprintf(url, sizeof(url),
        "https://11.0.1.3:50051/api/v1/keys/alice_client1/dec_keys?key_ID=%s",
        key_id
    );

    return fetch_from_api(
        url,
        "bob_client1.crt",
        "bob_client1.key",
        "rootCA_auth.crt",
        "enp3s0",
        key
    );
#else
    return parse_mock(key);
#endif
}