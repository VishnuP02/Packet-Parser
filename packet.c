#include "packet.h"

void pkt_init(pkt_parser_t *p)
{
    if (p == NULL) {
        return;
    }
    p->state = PKT_WAIT_START;
    p->len = 0;
    p->received = 0;
    p->sum = 0;
}

pkt_status_t pkt_feed(pkt_parser_t *p, uint8_t byte)
{
    if (p == NULL) {
        return PKT_ERR_NULL;
    }
    switch (p->state) {
    case PKT_WAIT_START:
        if (byte == PKT_START_BYTE) {
            p->state = PKT_WAIT_LEN;
        }
        return PKT_NONE;

    case PKT_WAIT_LEN:
        if (byte > PKT_MAX_PAYLOAD) {
            p->state = PKT_WAIT_START;
            return PKT_ERR_LENGTH;
        }
        p->len = byte;
        p->received = 0;
        p->sum = byte;
        p->state = (byte == 0) ? PKT_WAIT_CHECKSUM : PKT_WAIT_PAYLOAD;
        return PKT_NONE;

    case PKT_WAIT_PAYLOAD:
        p->payload[p->received++] = byte;
        p->sum = (uint8_t)(p->sum + byte);
        if (p->received == p->len) {
            p->state = PKT_WAIT_CHECKSUM;
        }
        return PKT_NONE;

    case PKT_WAIT_CHECKSUM:
    default:
        p->state = PKT_WAIT_START;
        if ((uint8_t)(p->sum + byte) != 0) {
            return PKT_ERR_CHECKSUM;
        }
        return PKT_FRAME_READY;
    }
}

uint8_t pkt_checksum(uint8_t len, const uint8_t *payload)
{
    uint8_t sum = len;
    uint8_t i;
    if (payload != NULL) {
        for (i = 0; i < len; i++) {
            sum = (uint8_t)(sum + payload[i]);
        }
    }
    return (uint8_t)(0u - sum);
}

size_t pkt_build(uint8_t *out, size_t out_size, const uint8_t *payload, uint8_t len)
{
    uint8_t i;
    if (out == NULL || len > PKT_MAX_PAYLOAD || (len > 0 && payload == NULL)) {
        return 0;
    }
    if (out_size < (size_t)len + 3u) {
        return 0;
    }
    out[0] = PKT_START_BYTE;
    out[1] = len;
    for (i = 0; i < len; i++) {
        out[2 + i] = payload[i];
    }
    out[2 + len] = pkt_checksum(len, payload);
    return (size_t)len + 3u;
}
