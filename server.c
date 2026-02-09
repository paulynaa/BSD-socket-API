// TCP serveris su select()
// aptarnauja kelis klientus vienu metu neblokuodamas kitų klientų ryšio

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

#define PORT 12345     // portas ant kurio klausys serveris
#define BACKLOG 10     // maksimalus waiting jungčių kiekis listen() eilėje
#define BUF_SIZE 2048  // buferis žinutėms

int main(void) {
    int listen_fd, new_fd, max_fd, i, rv;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen;
    fd_set master_set, read_fds;
    char buf[BUF_SIZE];

    // sukuriame TCP socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR leidžia pakartotinai naudoti tą patį portą po restart
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // paruošiame serverio adresą bind() funkcijai
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;          // IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // klausyti visų interfeisų
    serv_addr.sin_port = htons(PORT);        // konvertuojame į tinklo baitų tvarką, cia naudojama hton funkcija

    // pririšame socket prie porto
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // paleidžiame klausymą, serveris pasiruoses priimti jungtis
    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // paruošiame select() rinkinius
    FD_ZERO(&master_set);        // isvalome rinkini
    FD_SET(listen_fd, &master_set); // itraukiame klausymo socket
    max_fd = listen_fd;          // sekame didziausia FD reiksme selectui

    printf("Server launched. Listening to port %d\n", PORT);

    // pagrindinis ciklas
    while (1) {
        // selektui paduodamas read_fds yra kopija, nes select() jį modifikuoja
        read_fds = master_set;

        // laukiame įvykių — naujo prijungimo arba duomenų iš klientų
        rv = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (rv < 0) {
            if (errno == EINTR) continue;  // jei pertrauktas signalo — kartojame
            perror("select");
            break;
        }

        // tikriname visus failo deskriptorius iki max_fd
        for (i = 0; i <= max_fd; ++i) {

            // jei šis deskriptorius pasiruošęs skaitymui
            if (FD_ISSET(i, &read_fds)) {

                // jei įvyko aktyvumas ant listen_fd, tai naujas klientas
                if (i == listen_fd) {
                    addrlen = sizeof(cli_addr);

                    // priimame jungtį
                    new_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &addrlen);
                    if (new_fd >= 0) {

                        // itraukiame nauja klienta i bendra select rinkini
                        FD_SET(new_fd, &master_set);

                        // atnaujiname didžiausią FD reikšmę
                        if (new_fd > max_fd) max_fd = new_fd;

                        // išvedame informaciją apie prisijungusį klientą
                        printf("Naujas klientas: %s:%d (fd=%d)\n",
                               inet_ntoa(cli_addr.sin_addr),
                               ntohs(cli_addr.sin_port),
                               new_fd);
                    }

                // įvyko aktyvumas ant egzistuojančio kliento socket
                } else {
                    ssize_t nbytes = recv(i, buf, BUF_SIZE - 1, 0);

                    // jei klientas atsijungė arba įvyko klaida
                    if (nbytes <= 0) {
                        if (nbytes == 0)
                            printf("Client disconnected (fd=%d)\n", i);
                        else
                            perror("recv");

                        close(i);             // uždaryti socket
                        FD_CLR(i, &master_set); // išimti iš select rinkinio

                    } else {
                        // teisingai gauti duomenys
                        buf[nbytes] = '\0';

                        printf("[fd=%d] %s", i, buf);

                        // paprastas echo grąžina atgal tam pačiam klientui
                        send(i, buf, nbytes, 0);
                    }
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
