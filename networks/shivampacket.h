#ifndef SHIVAMPACKET_H
#define SHIVAMPACKET_H

#include <stdint.h>

#define PACKET_FRAGMENT_SIZE 3 // Define your fragment size
#define MAX_FRAGMENTS 100        // Maximum number of fragments

typedef struct {
    uint32_t fragment_seq_num;  // Sequence number of the fragment
    uint32_t fragment_total;    // Total number of fragments
    int packet_msg_id;          // Message ID
    char fragment_data[PACKET_FRAGMENT_SIZE]; // Data fragment
} PacketFragment;

typedef struct {
    uint32_t ack_sequence;    // Acknowledgment sequence number
    uint32_t nack_sequence;   // NACK (negative acknowledgment) sequence number
    int packet_msg_id;        // Message ID
    int is_nack_flag;         // Flag to indicate if it's a NACK
} AckResponse;

#endif
