#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"
#include "helpers.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define TIME 5000
#define MAXL 250

int main(int argc, char** argv) {
    msg t;
    struct s_pkt send_i;
    struct mk kermit;
    init(HOST, PORT);

    int seq = 0; // numar de secventa local
    
    /* Initializez campurile pachetului de Send-Init si il trimit catre
     receptor. In caz ca nu primesc raspuns in timp util, opresc transmisia. */
    init_s(&send_i, seq);
    init_mk(&kermit, seq, 'S');
    memcpy(kermit.data, &send_i, sizeof(send_i));
    seq = send_wait(kermit, sizeof(send_i) + 4, t, seq);
    if (seq == -1) {
        perror("Timeout - executie oprita la send-init \n");
        return -1;
    }
    
    /* Trimit urmatoarele pachete de tip "F", "D" si "Z". Pentru fiecare dintre
     acestea astept raspunsul receptorului, fac modificarile cerute, continui
     transmisia sau o opresc daca s-a retransmis acelasi pachet de 3 ori. */
    for (int i = 1; i < argc; i++) {

        // pachetul de tip "F" (File Header)
        init_mk(&kermit, seq, 'F');
        memcpy(kermit.data, argv[i], strlen(argv[i]) + 1);
        seq = send_wait(kermit, strlen(kermit.data) + 5, t, seq);
        if (seq == -1) {
            perror("Timeout - executie oprita \n");
            return -1;
        }

        FILE *f = fopen(argv[i], "rb");

        // pachetul de tip "D" (Date)
        while (1) {
            init_mk(&kermit, seq, 'D');
            int size = fread(kermit.data, 1, MAXL, f);
            if (size == 0)
                break;
            seq = send_wait(kermit, size + 4, t, seq);
            if (seq == -1) {
                perror("Timeout - executie oprita");
                return -1;
            }
        }

        // pachetul de tip "Z" (EOF)
        init_mk(&kermit, seq, 'Z');
        seq = send_wait(kermit, 4, t, seq);
        if (seq == -1) {
            perror("Timeout - executie oprita");
            return -1;
        }
    }

    /* Trimit pachetul de tip "B" (EOT) pana cand primesc ACK de la receptor,
     urmand apoi sa inchei transmisia */
    init_mk(&kermit, seq, 'B');
    seq = send_wait(kermit, 4, t, seq);
    if (seq == -1) {
        perror("Timeout - executie oprita");
        return -1;
    }
    return 0;
}
