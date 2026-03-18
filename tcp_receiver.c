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

    int sock, client;
    struct sockaddr_in server, cli;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&server, sizeof(server));
    listen(sock, 1);

    printf("Aguardando conexão...\n");

    socklen_t c = sizeof(cli);
    client = accept(sock, (struct sockaddr*)&cli, &c);

    char key_id[KEY_ID_SIZE] = {0};
    recv(client, key_id, KEY_ID_SIZE, 0);

    int len;
    recv(client, &len, sizeof(int), 0);

    unsigned char buffer[MAX_DATA_SIZE];
    recv(client, buffer, len, 0);

    printf("key_id recebido: %s\n", key_id);

    otp_key_t key;

    if (get_key_receiver(key_id, &key) != 0) {
        printf("Erro ao obter chave\n");
        return 1;
    }

    if (key.key_len < len) {
        printf("Erro: chave menor que mensagem\n");
        return 1;
    }

    otp_encrypt(buffer, key.key, len);

    buffer[len] = '\0';

    printf("Mensagem descriptografada: %s\n", buffer);

    close(client);
    close(sock);

    curl_global_cleanup();

    return 0;
}