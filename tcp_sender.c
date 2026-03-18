#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>

#include "crypto.h"
#include "key_fetch.h"
#include "common.h"

int main() {

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int sock;
    struct sockaddr_in server;

    //char message[MAX_DATA_SIZE] = "Olá Bob, OTP de Alice";

    char message[256];

    if (fgets(message, sizeof(message), stdin) == NULL) {
        return 0;
    }

    otp_key_t key;

    int len = strlen(message);

    if (get_key_sender(&key, len) != 0) {
        printf("Erro ao obter chave\n");
        return 1;
    }

    //int len = strlen(message);

    if (key.key_len < len) {
        printf("Erro: chave menor que mensagem (OTP inválido)\n");
        return 1;
    }

    otp_encrypt((unsigned char*)message, key.key, len);

    printf("Encrypted data enviado\n");
    printf("key_id=%s\n", key.key_id);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Message: %s", message);

    send(sock, key.key_id, KEY_ID_SIZE, 0);

    send(sock, &len, sizeof(int), 0);

    send(sock, message, len, 0);

    close(sock);

    curl_global_cleanup();

    return 0;
}