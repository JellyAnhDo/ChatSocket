#include <fcntl.h>
#include <stdint.h>    
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT_NUM 4000
#define SERVER_ADDR "127.0.0.1"

int server_fd;
pthread_t receive_thread, sent_thread;

int hextostring(char *in, int len, char *out)
{
    int i;

    memset(out, 0, sizeof(out));
    for (i = 0; i < len; i++)
    {
        sprintf(out, "%s%02hhx", out, in[i]);
    }
    return 0;
}

int stringtohex(char *in, int len, char *out)
{
    int i;
    int converter[105];
    converter['0'] = 0;
    converter['1'] = 1;
    converter['2'] = 2;
    converter['3'] = 3;
    converter['4'] = 4;
    converter['5'] = 5;
    converter['6'] = 6;
    converter['7'] = 7;
    converter['8'] = 8;
    converter['9'] = 9;
    converter['a'] = 10;
    converter['b'] = 11;
    converter['c'] = 12;
    converter['d'] = 13;
    converter['e'] = 14;
    converter['f'] = 15;

    memset(out, 0, sizeof(out));

    for (i = 0; i < len; i = i + 2)
    {
        char byte = converter[in[i]] << 4 | converter[in[i + 1]];
        out[i / 2] = byte;
    }
}

static void *receive_message(void *fun_arg)
{
    int *server_fd = (int *)fun_arg;
    uint16_t receive_byte;
    char buffer[1200], name[100], hex_message[1000], message[1200];
    int des_fd = open("/dev/aes_encrypt", O_RDWR);
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (read(*server_fd, &receive_byte, sizeof(receive_byte)) == 0)
        {
            printf("\033[1;33m\nDisconnect from server\033[0m\n");

            exit(0);
        }
        if (read(*server_fd, buffer, receive_byte) == 0)
        {
            printf("\033[1;33m\nDisconnect from server\033[0m\n");

            exit(0);
        }
        sscanf(buffer, "%[^:]: %[0-9abcdef]", name, hex_message);

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "decrypt\n%s", hex_message);
        write(des_fd, buffer, strlen(buffer));

        memset(hex_message, 0, sizeof(hex_message));
        read(des_fd, hex_message, sizeof(hex_message));

        stringtohex(hex_message, sizeof(hex_message), message);
        printf("%s: %s\n", name, message);
        
    }
    close(des_fd);
}

static void *send_message(void *fun_arg)
{
    int *server_fd = (int *)fun_arg;
    uint16_t sent_byte;
    char message[1000], hex_message[1000], encrypt_message[1200];

    int des_fd = open("/dev/aes_encrypt", O_RDWR);
    if (des_fd == -1) {
        printf("khong mo dc file driver\n");
        exit(0);
    }

    while (1)
    {
        memset(message, 0, sizeof(message));

        fgets(message, sizeof(message), stdin);
        message[strlen(message) - 1] = 0;

        if (strlen(message) != 0) 
        {
            hextostring(message, strlen(message), hex_message);
            sprintf(encrypt_message, "encrypt\n%s", hex_message);
            write(des_fd, encrypt_message, strlen(encrypt_message));

            memset(encrypt_message, 0, sizeof(encrypt_message));
            read(des_fd, encrypt_message, sizeof(encrypt_message));


            sent_byte = strlen(encrypt_message);
            write(*server_fd, &sent_byte, sizeof(sent_byte));
            write(*server_fd, encrypt_message, sent_byte);
        }
        

    }
    close(des_fd);
}

int setname(int server_fd)
{
    char name[100];
    uint16_t send_byte;

    printf("Chat name: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name) - 1] = 0;
    send_byte = strlen(name);
    write(server_fd, &send_byte, sizeof(send_byte));
    write(server_fd, name, send_byte);
}

int main()
{
    int status, login_status;
    struct sockaddr_in server_addr;
    char server_ip[INET_ADDRSTRLEN] = "127.0.0.1";
    int server_port;

    printf("===========================================\n");
    printf("\033[1;33mSetting up connection to the server...\033[0m\n\n");
    // Ask user for the IP address of the server
    printf("Enter server IP address: ");
    scanf("%s", server_ip);
    getchar(); // To consume the newline character left by scanf

    printf("Enter server port number: ");
    scanf("%d", &server_port);
    getchar(); // To consume the newline character left by scanf

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        fprintf(stderr, "create socket error\n");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) == -1)
    {
        fprintf(stderr, "server_addr fail\n");
        exit(0);
    }

    status = connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status == -1)
    {
        fprintf(stderr, "connect error\n");
        exit(0);
    }

    printf("\033[1;32mConnect to server!\033[0m\n");

    setname(server_fd);
    pthread_create(&receive_thread, NULL, receive_message, &server_fd);
    pthread_create(&sent_thread, NULL, send_message, &server_fd);
    while (1)
        sleep(1);

    return 0;
}
