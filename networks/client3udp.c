#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[MAX_BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_address);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Set server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    printf("Connected to the server.\n");

    sendto(sock, "replay_response", strlen("replay_response"), 0, (struct sockaddr *)&server_address, addr_len);

    while (1) {
        // Receive message from server
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_address, &addr_len) <= 0) {
            perror("recvfrom failed");
            break;
        }
        printf("%s", buffer);

        // If it's the player's turn, send move
        if (strstr(buffer, "Your turn!") != NULL) {
            int row, col;
            printf("Enter your move (row and column): ");
            scanf("%d %d", &row, &col);

            // Send move to server
            snprintf(buffer, sizeof(buffer), "%d %d", row, col);
            sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, addr_len);
        }

        // If the server asks if the player wants to play again
        if (strstr(buffer, "Do you want to play again? (yes/no)") != NULL) {
            char replay_response[10];
            printf("Enter 'yes' to play again or 'no' to exit: ");
            scanf("%s", replay_response);

            // Send replay response to server
            sendto(sock, replay_response, strlen(replay_response), 0, (struct sockaddr *)&server_address, addr_len);

            // If the player says "no," break out of the loop
            if (strncmp(replay_response, "no", 2) == 0) {
                printf("Exiting game. Goodbye!\n");
                break;
            }
        }
    }

    // Clean up
    close(sock);
    return 0;
}
