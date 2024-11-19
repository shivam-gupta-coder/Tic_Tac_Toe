#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "shivampacket.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define TIMEOUT_SEC 2
#define TIMEOUT_USEC 0
#define MAX_MESSAGE_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char message[MAX_MESSAGE_SIZE];

    // Create UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    while (1) {
        printf("Enter message (or 'quit' to exit): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0; // Remove trailing newline

        if (strcmp(message, "quit") == 0) {
            break;
        }

        int msg_len = strlen(message);
        int total_fragments = (msg_len + PACKET_FRAGMENT_SIZE - 1) / PACKET_FRAGMENT_SIZE;
        int *ack_received = calloc(total_fragments, sizeof(int));
        PacketFragment *fragments = malloc(total_fragments * sizeof(PacketFragment));

        // Prepare all fragments
        for (int i = 0; i < total_fragments; i++) {
            fragments[i].fragment_seq_num = i;
            fragments[i].fragment_total = total_fragments;
            strncpy(fragments[i].fragment_data, message + (i * PACKET_FRAGMENT_SIZE), PACKET_FRAGMENT_SIZE);
        }

        int all_acks_received = 0;
        int round = 1;

        while (!all_acks_received) {
            printf("Round %d: Sending all unacknowledged chunks\n", round);
            
            // Send all unacknowledged fragments
            for (int i = 0; i < total_fragments; i++) {
                if (!ack_received[i]) {
                    sendto(client_socket, &fragments[i], sizeof(PacketFragment), 0, 
                           (struct sockaddr *)&server_addr, sizeof(server_addr));
                    printf("Sent fragment %d of %d\n", i + 1, total_fragments);
                }
            }

            // Wait for ACKs
            fd_set readfds;
            struct timeval tv;
            FD_ZERO(&readfds);
            FD_SET(client_socket, &readfds);
            tv.tv_sec = TIMEOUT_SEC;
            tv.tv_usec = TIMEOUT_USEC;

            while (select(client_socket + 1, &readfds, NULL, NULL, &tv) > 0) {
                AckResponse ack;
                recvfrom(client_socket, &ack, sizeof(AckResponse), 0, NULL, NULL);
                if (ack.ack_sequence >= 0 && ack.ack_sequence < total_fragments) {
                    printf("Received ACK for fragment %d\n", ack.ack_sequence + 1);
                    ack_received[ack.ack_sequence] = 1;
                }
                FD_ZERO(&readfds);
                FD_SET(client_socket, &readfds);
                tv.tv_sec = 0;
                tv.tv_usec = 100000; // 0.1 second
            }

            // Check if all ACKs received
            all_acks_received = 1;
            for (int i = 0; i < total_fragments; i++) {
                if (!ack_received[i]) {
                    all_acks_received = 0;
                    break;
                }
            }

            if (!all_acks_received) {
                printf("Some fragments not acknowledged, starting next round...\n");
            }
            
            round++;
        }

        printf("All fragments acknowledged successfully.\n");

        free(ack_received);
        free(fragments);
    }

    close(client_socket);
    return 0;
}
