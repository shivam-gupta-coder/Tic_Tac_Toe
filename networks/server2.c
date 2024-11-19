#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BOARD_SIZE 9 // 9 cells for the board

char board[BOARD_SIZE + 1];  // The Tic-Tac-Toe board
int player_sockets[2];        // Store the two player sockets

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

void send_to_both_clients(const char *message) {
    send(player_sockets[0], message, strlen(message), 0);
    send(player_sockets[1], message, strlen(message), 0);
}

void send_board_state() {
    char buf[256];
    print_board(buf);
    printf("%s", buf);
    send_to_both_clients(buf);
}

int main() {
    int server_fd, new_socket, addrlen;
    struct sockaddr_in address;

    // Creating socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Binding to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    addrlen = sizeof(address);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Accept Player 1
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    player_sockets[0] = new_socket;
    printf("Player 1 connected\n");

    // Accept Player 2
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    player_sockets[1] = new_socket;
    printf("Player 2 connected\n");

    int row, col, index;
    char buffer[1024];
    char move_msg[50];
    int winner = 0;
    int current_player;

    // Start of the main game loop
    game_loop:  // Label for goto
        reset_board();  // Reset the board for a new game
        current_player = 1;  // Player 1 starts
        winner = 0;  // Reset the winner

        while (winner == 0) {
            if (current_player == 1) {
                send(player_sockets[0], "Your turn! Enter row (1-3) and col (1-3): ", 50, 0);
                send(player_sockets[1], "Waiting for Player 1 to make a move...\n", 50, 0);
                recv(player_sockets[0], buffer, sizeof(buffer), 0);
            } else {
                send(player_sockets[1], "Your turn! Enter row (1-3) and col (1-3): ", 50, 0);
                send(player_sockets[0], "Waiting for Player 2 to make a move...\n", 50, 0);
                recv(player_sockets[1], buffer, sizeof(buffer), 0);
            }

            sscanf(buffer, "%d %d", &row, &col);
            index = (row - 1) * 3 + (col - 1);

            if (index >= 0 && index < BOARD_SIZE && board[index] == '-') {
                board[index] = (current_player == 1) ? 'X' : 'O';
                send_board_state();
                current_player = (current_player == 1) ? 2 : 1;  // Switch turns
            } else {
                send(player_sockets[current_player - 1], "Invalid move! Try again.\n", 25, 0);
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
                send_to_both_clients(end_msg);
            }
            
        }

        // Ask if players want to play again
        char replay_msg[] = "Do you want to play again? (yes/no)\n";
        send_to_both_clients(replay_msg);

        // Receive response from both players
        char response1[10] = {0};  // Initialize to zero
        char response2[10] = {0};  // Initialize to zero

        // Receive response from Player 1
        recv(player_sockets[0], response1, sizeof(response1) - 1, 0);
        response1[strcspn(response1, "\n")] = 0;  // Remove newline character

        // Receive response from Player 2
        recv(player_sockets[1], response2, sizeof(response2) - 1, 0);
        response2[strcspn(response2, "\n")] = 0;  // Remove newline character

        // Check if both players want to play again
        if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "yes", 3) == 0) {
            goto game_loop;  // Jump back to the start of the game loop
        }
        else if(strncmp(response1, "yes", 3) == 0 && strncmp(response2, "no", 2) == 0) {
            char msg[]="Your opponentn doesn't want to play\n";
            send(player_sockets[0],msg,sizeof(msg),0);
        }
        else if(strncmp(response1, "no", 2) == 0 && strncmp(response2, "yes", 3) == 0) {
            char msg[]="Your opponentn doesn't want to play\n";
            send(player_sockets[1],msg,sizeof(msg),0);
        }

    // Clean up
    close(player_sockets[0]);
    close(player_sockets[1]);
    close(server_fd);

    return 0;
}
