// Paprastas TCP klientas su select()

#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"   // IP adresas, prie kurio klientas jungiasi
#define SERVER_PORT 12345       // serverio portas
#define BUF_SIZE 2048           // buferio dydis zinutėms

int main(void) {
    int sockfd, rv;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    char buf[BUF_SIZE];

    // sukuriame kliento TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    // paruošiame serverio adreso struktūrą
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                // IPv4
    serv_addr.sin_port = htons(SERVER_PORT);       // portas tinklo baitu tvarkoje

    // konvertuojame tekstinį IP į dvejetainį formatą
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    // jungiamės prie serverio
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server. Send message:\n");

    // pagrindinis select ciklas
    while (1) {
        // nustatome, kuriuos FD stebėsime
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // vartotojo įvestis
        FD_SET(sockfd, &read_fds);       // duomenys iš serverio

        // didžiausias FD reikalingas select()
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        // laukiame, kol bus galima skaityti arba iš stdin, arba iš socket
        rv = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
        if (rv < 0) { perror("select"); break; }

        // ivestis iš vartotojo (stdin)
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {

            // jei gauname EOF (Ctrl+D)
            if (!fgets(buf, BUF_SIZE, stdin)) {
                // uždaryti siuntimo kryptį, serveris gaus EOF
                shutdown(sockfd, SHUT_WR);
            } else {
                // išsiunčiame vartotojo įvestą tekstą serveriui
                send(sockfd, buf, strlen(buf), 0);
            }
        }

        // duomenys atėjo iš serverio socket
        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t n = recv(sockfd, buf, BUF_SIZE - 1, 0);

            // jei serveris uždarė ryšį arba įvyko klaida
            if (n <= 0) {
                printf("Server closed connection.\n");
                break;
            }

            // baigiamasis nulinis simbolis, kad spausdinti kaip tekstą
            buf[n] = '\0';
            printf("Received: %s", buf);
        }
    }

    // uždaryti socket ir išeiti
    close(sockfd);
    return 0;
}
