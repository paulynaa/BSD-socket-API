//  peer-to-peer mazgas, veikia kaip serveris ir klientas

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

#define BUF_SIZE 2048
#define LISTEN_BACKLOG 10

// pagalbinė funkcija suranda didžiausią fd master rinkinyje
// reikalinga, kai select reikia žinoti maxfd+1
static int recompute_maxfd(fd_set *master, int current_max) {
    for (int fd = current_max; fd >= 0; --fd) {
        if (FD_ISSET(fd, master)) return fd;
    }
    return 0;
}

// sukuria klausymo lizdą nurodytam portui
int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // leidžia greičiau panaudoti tą patį portą po perkrovimo
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    // nustatome serverio adresą
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    // pririšame socketą prie adreso
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    // paleidžiame listen režimą, kad galėtume priimti prisijungimus
    if (listen(fd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

// sukuria aktyvų prisijungimą prie peer
int connect_to_peer(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // paruošiame remote adresą
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    // konvertuojame tekstinį IP į binary formą
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", ip);
        close(fd);
        return -1;
    }

    // bandome prisijungti prie peer
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    // išvedame patvirtinimą
    char peerstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, peerstr, sizeof(peerstr));
    printf("connected to peer %s:%d (fd=%d)\n", peerstr, ntohs(addr.sin_port), fd);

    return fd;
}

int main(int argc, char *argv[]) {
    // tikriname argumentus
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <listen_port> [peer_ip peer_port]\n", argv[0]);
        return 1;
    }

    int listen_port = atoi(argv[1]);
    if (listen_port <= 0) {
        fprintf(stderr, "invalid listen port\n");
        return 1;
    }

    // sukuriame klausymo socketą
    int listen_fd = make_listen_socket(listen_port);
    if (listen_fd < 0) return 1;

    // jei nurodytas listen_port, peer_ip ir peer_port, pograma iskart bando prisijungti prie nurodyto peero
    int initial_peer_fd = -1;
    if (argc == 4) {
        initial_peer_fd = connect_to_peer(argv[2], atoi(argv[3]));
    }

    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // įkeliame klausymo socketą ir stdin į master
    FD_SET(listen_fd, &master);
    FD_SET(STDIN_FILENO, &master);

    int maxfd = listen_fd;
    if (STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;

    // jei iš karto prisijungėme prie peer
    if (initial_peer_fd >= 0) {
        FD_SET(initial_peer_fd, &master);
        if (initial_peer_fd > maxfd) maxfd = initial_peer_fd;
    }

    char buf[BUF_SIZE];

    printf("p2p node running, listening on port %d\n", listen_port);
    
    // pagrindinis select ciklas
    while (1) {
        // read_fds bus modifikuojama select(), todėl kopijuojame master
        read_fds = master;

        int rv = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
        if (rv < 0) {
            if (errno == EINTR) continue; // signalai, ignoruojame
            perror("select");
            break;
        }

        // patikriname, ar yra naujų incoming connection (listen_fd)
        if (FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in cli;
            socklen_t len = sizeof(cli);
            int fd = accept(listen_fd, (struct sockaddr *)&cli, &len);
            if (fd < 0) {
                perror("accept");
            } else {
                // įtraukiame naują peer į master fd rinkinį
                FD_SET(fd, &master);
                if (fd > maxfd) maxfd = fd;

                char addrbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli.sin_addr, addrbuf, sizeof(addrbuf));
                printf("accepted peer %s:%d (fd=%d)\n", addrbuf, ntohs(cli.sin_port), fd);
            }
        }

        // patikriname vartotojo įvestį iš stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (!fgets(buf, sizeof(buf), stdin)) {
                // stdin uždarytas
                printf("EOF exit\n");
                break;
            }

            // nuimame newline
            size_t len = strlen(buf);
            if (len && buf[len-1] == '\n') buf[len-1] = '\0';

            // komanda connect <ip> <port>
            if (strncmp(buf, "connect ", 8) == 0) {
                char ip[64];
                int port;
                if (sscanf(buf + 8, "%63s %d", ip, &port) == 2) {
                    int fd = connect_to_peer(ip, port);
                    if (fd >= 0) {
                        FD_SET(fd, &master);
                        if (fd > maxfd) maxfd = fd;

                        // siunčiam paprastą patvirtinimo žinutę
                        const char *hello = " peer connected\n";
                        send(fd, hello, strlen(hello), 0);
                    }
                } else {
                    printf("Usage: connect <ip> <port>\n");
                }
                continue;
            }

            // komanda peers parodo visus aktyvius fd
            else if (strcmp(buf, "peers") == 0) {
                printf("active fds: ");
                for (int fd = 0; fd <= maxfd; ++fd) {
                    if (FD_ISSET(fd, &master)) {
                        if (fd == listen_fd) printf("[listen:%d] ", fd);
                        else if (fd == STDIN_FILENO) printf("[stdin] ");
                        else printf("%d ", fd);
                    }
                }
                printf("\n");
                continue;
            }

            // komanda quit arba exit
            else if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0) {
                printf("exiting on user request...\n");
                break;
            }

            // jei ne komanda: tai yra žinutė broadcast visiems peers
            size_t to_send = strlen(buf);
            if (to_send > 0) {
                // pridedame newline atvaizdavimui
                char sendbuf[BUF_SIZE];
                snprintf(sendbuf, sizeof(sendbuf), "%s\n", buf);
                size_t sendlen = strlen(sendbuf);

                for (int fd = 0; fd <= maxfd; ++fd) {
                    if (!FD_ISSET(fd, &master)) continue;
                    if (fd == STDIN_FILENO || fd == listen_fd) continue;

                    ssize_t s = send(fd, sendbuf, sendlen, 0);
                    if (s < 0) {
                        // jei siuntimas nepavyko uždarome peer
                        fprintf(stderr, "send to fd %d failed: %s\n", fd, strerror(errno));
                        close(fd);
                        FD_CLR(fd, &master);
                    }
                }

                // perskaičiuojame maxfd
                maxfd = recompute_maxfd(&master, maxfd);
            }
        }

        // apdorojame atėjusį srautą iš visų peer socketų
        for (int fd = 0; fd <= maxfd; ++fd) {
            if (fd == listen_fd || fd == STDIN_FILENO) continue;
            if (!FD_ISSET(fd, &read_fds)) continue;

            ssize_t n = recv(fd, buf, BUF_SIZE - 1, 0);
            if (n <= 0) {
                // jei recv==0, peer uždarė ryšį
                if (n == 0) {
                    printf("peer fd=%d closed connection\n", fd);
                } else {
                    perror("recv");
                }
                close(fd);
                FD_CLR(fd, &master);

                // perskaičiuojame maxfd
                maxfd = recompute_maxfd(&master, maxfd);
            } else {
                buf[n] = '\0';
                // išvedame gautą žinutę su fd, kad matytume iš kur atėjo
                printf("[fd %d] %s", fd, buf);
            }
        }
    }

    // išeinant uždarome visus fd
    for (int fd = 0; fd <= maxfd; ++fd) {
        if (FD_ISSET(fd, &master)) close(fd);
    }
    close(listen_fd);

    return 0;
}
