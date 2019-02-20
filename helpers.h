#ifndef HLP
#define HLP

void init_s(struct s_pkt *package, int seq) {
    package->MAXL = 250;
    package->TIME = 5;
    package->NPAD = 0x00;
    package->PADC = 0x00;
    package->EOL = 0x0D;
    package->QCTL = 0x00;
    package->QBIN = 0x00;
    package->CHKT = 0x00;
    package->REPT = 0x00;
    package->CAPA = 0x00;
    package->R = 0x00;
}

void init_mk(struct mk *package, int seq, char type) {
	package->SOH = 0x01;
	memset(package->data, 0, sizeof(package->data));
	package->MARK = 0x0D;
	package->TYPE = type;
	package->SEQ = seq;
}

void init_dv(struct no_data *package, int seq, char type) {
    package->SOH = 0x01;
    package->MARK = 0x0D;
    package->TYPE = type;
    package->SEQ = seq;
    package->LEN = 5;
}
// dim -> lungimea ocupata de primiele 4 campuri si zona de date
int send_wait(struct mk kermit, int dim, msg t, int seq) {
    // Construiesc pachetul si il trimit
	kermit.LEN = dim + 1;
	kermit.CHECK = crc16_ccitt(&kermit, dim);
	memset(t.payload, 0, sizeof(t.payload));
	memcpy(t.payload, &kermit, dim);
	memcpy(t.payload + dim, &kermit.CHECK, 2);
	memcpy(t.payload + dim + 2, &kermit.MARK, 1);
	t.len = dim + 3;
	send_message(&t);

	msg *r;
	int ct_ret = 0;
	while (1) {
		r = receive_message_timeout(5000);
		if (r == NULL) {
            send_message(&t);
            if (ct_ret == 3)
                return -1;
            ct_ret++;
		}
		else if (r->payload[2] == (seq + 1) % 64) {
			seq = (r->payload[2] + 1) % 64;
			if (r->payload[3] == 'Y') {
               	break;
			}
			else if (r->payload[3] == 'N') {
                /* Reconstruiesc pachetul, crescand numarul de secventa si
                actualizand campul CHECK */
                ct_ret = 0; // reinitializez
				kermit.SEQ = seq;
                kermit.CHECK = crc16_ccitt(&kermit, dim);
                memcpy(t.payload, &kermit, dim);
                memcpy(t.payload + dim, &kermit.CHECK, 2);
				memcpy(t.payload + dim + 2, &kermit.MARK, 1);
                t.len = dim + 3;
                send_message(&t);
			}
		}
	}
	return seq;
}

msg receive_send_init(int *seq, msg t, int *exit) {
    // aceste variabile au aceeasi insemnatate ca si cele din receiver
    int ct_ret;
    unsigned short check, crc;
    unsigned char len;
    msg *r; 
    r = receive_message_timeout(15000);
    if (r == NULL) {
        *exit = -1;
        return t;
    } else {
        while (1) {
            if (r == NULL) {
                if (ct_ret == 3) {
                    *exit = -1;
                    return t;
                }
                ct_ret++;
            } else {
                memcpy(t.payload, r->payload, r->len);
                if (r->payload[2] == (*seq + 1)) {
                    // Construim adecvat raspunsul pentru Send-Init si trimitem
                    memcpy(t.payload, r->payload, r->len);
                    *seq = (t.payload[2] + 1) % 64;
                    t.payload[2] = *seq;
                    t.len = r->len;
                    crc = crc16_ccitt(r->payload, r->len - 3);
                    memcpy(&len, r->payload + 1, 1);
                    memcpy(&check, r->payload + len - 1, 2);
                    
                    ct_ret = 0; // reinitializez
                    if (crc == check) {
                        t.payload[3] = 'Y';
                        send_message(&t);
                        break;
                    } else {
                        t.payload[3] = 'N';
                        send_message(&t);
                    }
                }
            }
            r = receive_message_timeout(5000);
        }
    }
    return t;
}

#endif
