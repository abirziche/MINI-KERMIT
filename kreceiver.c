#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include "helpers.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    msg *r, t;
    struct no_data res;
    
    init(HOST, PORT);

    // ct_ret -> tine evidenta retransmiterii unui pachet in caz de timeout
    // seq -> numar de secventa local, incepe de la -1 din cauza verificarilor
    // exit -> folosita in cazul in care nu s-a primit pachetul de send-init
    int ct_ret = 0, seq = -1, exit = 0;
    // crc -> aici se pastreaza calculul crc-ului cu ajutorul functiei
    // check -> campul CHECK din pachet
    unsigned short crc, check;
    // len -> campul LEN din pachet
    unsigned char len, MAXL;
    char TIME, nume_fisier[20] = "recv_";
    FILE *f;

    // Primesc pachetul de Send-Init si pastrez ce am nevoie (TIME && MAXL)
    t = receive_send_init(&seq, t, &exit);
    if (exit == -1) {
        return -1;
    }
    
    MAXL = t.payload[4];
    TIME = t.payload[5];
    char buf[MAXL];

    while (1) {
        r = receive_message_timeout(TIME * 1000);
        if (r == NULL) {
            send_message(&t);
            if (ct_ret == 3)
                return -1;
            ct_ret++;
        } else {
            if (r->payload[2] == (seq + 1) % 64) {
                
                /* Scot din pachet campurile de care am nevoie pentru a afla
                daca a ajuns corect sau nu */
                memcpy(&len, r->payload + 1, 1);
                crc = crc16_ccitt(r->payload, len - 1);
                seq = (r->payload[2] + 1) % 64;
                memcpy(&check, r->payload + len - 1, 2);
                memset(buf, 0, 250);
                
                ct_ret = 0; // reinitializez
                if (crc == check) {
                
                    if (r->payload[3] == 'F') {
                        memcpy(buf, r->payload + 4, len - 5);
                        strcat(nume_fisier, buf);
                        f = fopen(nume_fisier, "wb");
                        memset(nume_fisier, 0, sizeof(nume_fisier));
                        strcpy(nume_fisier, "recv_");
                    }

                    if (r->payload[3] == 'D') {
                        memcpy(buf, r->payload + 4, len - 5);
                        fwrite(buf, 1, len - 5, f);
                    }

                    if (r->payload[3] == 'Z') {
                        fclose(f);
                    }
                
                    init_dv(&res, seq, 'Y');
                    memcpy(t.payload, &res, sizeof(res));
                    t.len = sizeof(res);
                    send_message(&t);
                
                    if (r->payload[3] == 'B') {
                        break;
                    }
                
                } else {
                    init_dv(&res, seq, 'N');
                    memcpy(t.payload, &res, sizeof(res));
                    t.len = sizeof(res);
                    send_message(&t);
                }
            } 
        }
    }

    return 0;
}
