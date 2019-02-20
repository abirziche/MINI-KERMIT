#ifndef LIB
#define LIB

typedef struct {
    int len;
    char payload[1400];
} msg;

struct __attribute__((__packed__)) s_pkt {
	unsigned char MAXL;
	char TIME;
	char NPAD;
	char PADC;
	char EOL;
	char QCTL;
	char QBIN;
	char CHKT;
	char REPT;
	char CAPA;
	char R;
};

struct __attribute__((__packed__)) mk {
	char SOH;
	unsigned char LEN;
	char SEQ;
	char TYPE;
	char data[250];
	unsigned short CHECK;
	char MARK;
};

struct __attribute__((__packed__)) no_data {
	char SOH;
	unsigned char LEN;
	char SEQ;
	char TYPE;
	unsigned short CHECK;
	char MARK;
};

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif
