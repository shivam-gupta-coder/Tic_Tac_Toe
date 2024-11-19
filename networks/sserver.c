#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include "shivampacket.h"

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024
#define MAX_FRAGMENTS 32

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // Create UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(server_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    srand(time(NULL)); // Initialize random seed

    while (1) {
        PacketFragment *fragments = malloc(MAX_FRAGMENTS * sizeof(PacketFragment));
        int *fragment_received = calloc(MAX_FRAGMENTS, sizeof(int));
        int *ack_sent = calloc(MAX_FRAGMENTS, sizeof(int));
        int total_fragments = 0;
        int all_fragments_received = 0;
        int round = 1;

        while (!all_fragments_received) {
            printf("Round %d: Receiving chunks\n", round);

            PacketFragment fragment;
            ssize_t recv_len = recvfrom(server_socket, &fragment, sizeof(PacketFragment), 0, 
                                        (struct sockaddr *)&client_addr, &client_addr_len);
            
            if (recv_len < 0) {
                perror("recvfrom failed");
                continue;
            }

            if (fragment.fragment_seq_num < MAX_FRAGMENTS) {
                memcpy(&fragments[fragment.fragment_seq_num], &fragment, sizeof(PacketFragment));
                fragment_received[fragment.fragment_seq_num] = 1;
                total_fragments = fragment.fragment_total;

                printf("Received fragment %d of %d\n", fragment.fragment_seq_num + 1, total_fragments);

                // Randomly decide whether to send ACK (70% chance)
                if (!ack_sent[fragment.fragment_seq_num] && (rand() % 100 < 70)) {
                    AckResponse ack;
                    ack.ack_sequence = fragment.fragment_seq_num;
                    sendto(server_socket, &ack, sizeof(AckResponse), 0, 
                           (struct sockaddr *)&client_addr, client_addr_len);
                    printf("Sent ACK for fragment %d\n", fragment.fragment_seq_num + 1);
                    ack_sent[fragment.fragment_seq_num] = 1;
                } else {
                    printf("Skipped sending ACK for fragment %d\n", fragment.fragment_seq_num + 1);
                }
            }

            // Check if all fragments are received and acknowledged
            all_fragments_received = 1;
            for (int i = 0; i < total_fragments; i++) {
                if (!fragment_received[i] || !ack_sent[i]) {
                    all_fragments_received = 0;
                    break;
                }
            }

            round++;
        }

        // Reconstruct and print the message
        printf("Received message: ");
        for (int i = 0; i < total_fragments; i++) {
            printf("%s", fragments[i].fragment_data);
        }
        printf("\n------ End of message ------\n");

        free(fragments);
        free(fragment_received);
        free(ack_sent);
    }

    close(server_socket);
    return 0;
}
