#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BOARD_SIZE 9 // 9 cells for the board
#define BUFFER_SIZE 1024

char board[BOARD_SIZE + 1];    // The Tic-Tac-Toe board
struct sockaddr_in player_addrs[2]; // Store the two player addresses
int player_count = 0;          // Track how many players are connected

void reset_board() {
    strcpy(board, "---------");
}

void print_board(char *buf) {
    sprintf(buf, "\n %c | %c | %c\n---|---|---\n %c | %c | %c\n---|---|---\n %c | %c | %c\n", 
            board[0], board[1], board[2], 
            board[3], board[4], board[5], 
            board[6], board[7], board[8]);
}

int check_winner() {
    char winning_combinations[8][3] = {
        {board[0], board[1], board[2]},
        {board[3], board[4], board[5]},
        {board[6], board[7], board[8]},
        {board[0], board[3], board[6]},
        {board[1], board[4], board[7]},
        {board[2], board[5], board[8]},
        {board[0], board[4], board[8]},
        {board[2], board[4], board[6]}
    };

    for (int i = 0; i < 8; i++) {
        if (winning_combinations[i][0] == winning_combinations[i][1] && 
            winning_combinations[i][1] == winning_combinations[i][2] && 
            winning_combinations[i][0] != '-') {
            return (winning_combinations[i][0] == 'X') ? 1 : 2;  // Return the winner (1 or 2)
        }
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == '-') {
            return 0;  // Game not over
        }
    }
    return 3;  // Draw
}

void send_to_both_clients(const char *message, int sockfd) {
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&player_addrs[0], sizeof(player_addrs[0]));
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&player_addrs[1], sizeof(player_addrs[1]));
}

void send_board_state(int sockfd) {
    char buf[256];
    print_board(buf);
    printf("%s", buf);
    send_to_both_clients(buf, sockfd);
}

int main() {
    int sockfd, len;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    int row, col, index;
    int winner = 0;
    int current_player;
    socklen_t addr_len = sizeof(client_addr);

    // Creating socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Binding to the port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Wait for two players to connect by sending their initial names or messages
    // for (int i = 0; i < 2; i++) {
    //     recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
    //     player_addrs[i] = client_addr;  // Store the client's address
    //     printf("Player %d connected\n", i + 1);
    //     player_count++;
    // }
    printf("Waiting for Player 1...\n");
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
    player_addrs[0] = client_addr;  // Store Player 1's address
    printf("Player 1 connected\n");
    player_count++;

    // Wait for player 2
    printf("Waiting for Player 2...\n");
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
    player_addrs[1] = client_addr;  // Store Player 2's address
    printf("Player 2 connected\n");
    player_count++;

    // Start of the main game loop
    game_loop:  // Label for goto
        reset_board();  // Reset the board for a new game
        current_player = 1;  // Player 1 starts
        winner = 0;  // Reset the winner

        while (winner == 0) {
            if (current_player == 1) {
                sendto(sockfd, "Your turn! Enter row (1-3) and col (1-3): ", 50, 0, 
                       (struct sockaddr *)&player_addrs[0], sizeof(player_addrs[0]));
                sendto(sockfd, "Waiting for Player 1 to make a move...\n", 50, 0, 
                       (struct sockaddr *)&player_addrs[1], sizeof(player_addrs[1]));
                recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player_addrs[0], &addr_len);
            } else {
                sendto(sockfd, "Your turn! Enter row (1-3) and col (1-3): ", 50, 0, 
                       (struct sockaddr *)&player_addrs[1], sizeof(player_addrs[1]));
                sendto(sockfd, "Waiting for Player 2 to make a move...\n", 50, 0, 
                       (struct sockaddr *)&player_addrs[0], sizeof(player_addrs[0]));
                recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player_addrs[1], &addr_len);
            }

            sscanf(buffer, "%d %d", &row, &col);
            index = (row - 1) * 3 + (col - 1);

            if (index >= 0 && index < BOARD_SIZE && board[index] == '-') {
                board[index] = (current_player == 1) ? 'X' : 'O';
                send_board_state(sockfd);
                current_player = (current_player == 1) ? 2 : 1;  // Switch turns
            } else {
                sendto(sockfd, "Invalid move! Try again.\n", 25, 0, 
                       (struct sockaddr *)&player_addrs[current_player - 1], sizeof(player_addrs[current_player - 1]));
                continue;  // Ask for another input if the move is invalid
            }

            // Check if the game has ended
            winner = check_winner();
            if (winner != 0) {
                char end_msg[50];
                if (winner == 1 || winner == 2) {
                    sprintf(end_msg, "Player %d wins!\n", winner);
                } else {
                    sprintf(end_msg, "It's a draw!\n");
                }
                printf("%s\n", end_msg);
                send_to_both_clients(end_msg, sockfd);
            }
        }

        // Ask if players want to play again
        char replay_msg[] = "Do you want to play again? (yes/no)\n";
        send_to_both_clients(replay_msg, sockfd);

        // Receive response from both players
        char response1[10] = {0};  // Initialize to zero
        char response2[10] = {0};  // Initialize to zero

        // Receive response from Player 1
        recvfrom(sockfd, response1, sizeof(response1) - 1, 0, (struct sockaddr *)&player_addrs[0], &addr_len);
        response1[strcspn(response1, "\n")] = 0;  // Remove newline character

        // Receive response from Player 2
        recvfrom(sockfd, response2, sizeof(response2) - 1, 0, (struct sockaddr *)&player_addrs[1], &addr_len);
        response2[strcspn(response2, "\n")] = 0;  // Remove newline character

        // Check if both players want to play again
        if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "yes", 3) == 0) {
            goto game_loop;  // Jump back to the start of the game loop
        } else {
            send_to_both_clients("One or both players don't want to play again. Exiting...\n", sockfd);
        }

    // Clean up
    close(sockfd);

    return 0;
}
