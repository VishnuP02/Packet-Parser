#ifndef PACKET_H
#define PACKET_H

#include <stddef.h>
#include <stdint.h>

/*
 * Byte-at-a-time frame parser (state machine).
 *
 * Frame layout:  [0x7E] [LEN] [PAYLOAD x LEN] [CHECKSUM]
 *   LEN       payload length, 0..PKT_MAX_PAYLOAD
 *   CHECKSUM  chosen so that (LEN + all payload bytes + CHECKSUM) mod 256 == 0
 */

#define PKT_START_BYTE   0x7Eu
#define PKT_MAX_PAYLOAD  16u

typedef enum {
    PKT_NONE = 0,         /* byte consumed, no frame yet */
    PKT_FRAME_READY = 1,  /* a complete, valid frame is available */
    PKT_ERR_LENGTH = -1,  /* length field exceeded PKT_MAX_PAYLOAD; parser resynchronized */
    PKT_ERR_CHECKSUM = -2,/* checksum mismatch; frame discarded, parser resynchronized */
    PKT_ERR_NULL = -3     /* NULL parser */
} pkt_status_t;

typedef enum { PKT_WAIT_START, PKT_WAIT_LEN, PKT_WAIT_PAYLOAD, PKT_WAIT_CHECKSUM } pkt_state_t;

typedef struct {
    pkt_state_t state;
    uint8_t len;
    uint8_t received;
    uint8_t sum;
    uint8_t payload[PKT_MAX_PAYLOAD];
} pkt_parser_t;

void pkt_init(pkt_parser_t *p);
pkt_status_t pkt_feed(pkt_parser_t *p, uint8_t byte);
uint8_t pkt_checksum(uint8_t len, const uint8_t *payload);
/* Builds a frame into out (size >= len + 3). Returns bytes written, or 0 on bad arguments. */
size_t pkt_build(uint8_t *out, size_t out_size, const uint8_t *payload, uint8_t len);

#endif
