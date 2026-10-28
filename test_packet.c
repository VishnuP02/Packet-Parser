/* Requirements-based tests. Each check names the requirement (see README.md) it verifies. */
#include <stdio.h>
#include <string.h>
#include "packet.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(req, cond)                                                     \
    do {                                                                     \
        tests_run++;                                                         \
        if (!(cond)) {                                                       \
            tests_failed++;                                                  \
            printf("FAIL [%s] %s:%d: %s\n", req, __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

/* Feeds a whole buffer; returns the status of the last byte and stores the last non-NONE status. */
static pkt_status_t feed_all(pkt_parser_t *p, const uint8_t *buf, size_t n, pkt_status_t *last_event)
{
    size_t i;
    pkt_status_t s = PKT_NONE;
    for (i = 0; i < n; i++) {
        s = pkt_feed(p, buf[i]);
        if (s != PKT_NONE) {
            *last_event = s;
        }
    }
    return s;
}

static void test_valid_frame(void)
{
    const uint8_t payload[3] = {0x10, 0x20, 0x30};
    uint8_t frame[19];
    pkt_parser_t p;
    pkt_status_t ev = PKT_NONE;
    size_t n = pkt_build(frame, sizeof frame, payload, 3);
    CHECK("PKT-001", n == 6);
    pkt_init(&p);
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-001", ev == PKT_FRAME_READY);
    CHECK("PKT-001", p.len == 3 && memcmp(p.payload, payload, 3) == 0);
}

static void test_empty_and_max_payload(void)
{
    uint8_t big[PKT_MAX_PAYLOAD];
    uint8_t frame[PKT_MAX_PAYLOAD + 3];
    pkt_parser_t p;
    pkt_status_t ev = PKT_NONE;
    size_t n, i;
    for (i = 0; i < sizeof big; i++) {
        big[i] = (uint8_t)(0xA0u + i);
    }
    n = pkt_build(frame, sizeof frame, NULL, 0);
    pkt_init(&p);
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-002", n == 3 && ev == PKT_FRAME_READY && p.len == 0);

    ev = PKT_NONE;
    n = pkt_build(frame, sizeof frame, big, PKT_MAX_PAYLOAD);
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-002", n == PKT_MAX_PAYLOAD + 3 && ev == PKT_FRAME_READY);
    CHECK("PKT-002", memcmp(p.payload, big, PKT_MAX_PAYLOAD) == 0);
}

static void test_checksum_error_and_recovery(void)
{
    const uint8_t payload[2] = {1, 2};
    uint8_t frame[5];
    pkt_parser_t p;
    pkt_status_t ev = PKT_NONE;
    size_t n = pkt_build(frame, sizeof frame, payload, 2);
    pkt_init(&p);
    frame[3] ^= 0x01u; /* corrupt one payload byte */
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-003", ev == PKT_ERR_CHECKSUM);
    /* a good frame must parse immediately afterward */
    n = pkt_build(frame, sizeof frame, payload, 2);
    ev = PKT_NONE;
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-003", ev == PKT_FRAME_READY);
}

static void test_oversize_length(void)
{
    pkt_parser_t p;
    pkt_status_t s;
    pkt_init(&p);
    pkt_feed(&p, PKT_START_BYTE);
    s = pkt_feed(&p, PKT_MAX_PAYLOAD + 1);
    CHECK("PKT-004", s == PKT_ERR_LENGTH);
    CHECK("PKT-004", p.state == PKT_WAIT_START);
}

static void test_garbage_before_start(void)
{
    const uint8_t garbage[5] = {0x00, 0xFF, 0x12, 0x34, 0x56};
    const uint8_t payload[1] = {0x42};
    uint8_t frame[4];
    pkt_parser_t p;
    pkt_status_t ev = PKT_NONE, s;
    size_t i, n = pkt_build(frame, sizeof frame, payload, 1);
    int ok = 1;
    pkt_init(&p);
    for (i = 0; i < sizeof garbage; i++) {
        s = pkt_feed(&p, garbage[i]);
        ok &= (s == PKT_NONE);
    }
    CHECK("PKT-005", ok);
    feed_all(&p, frame, n, &ev);
    CHECK("PKT-005", ev == PKT_FRAME_READY && p.payload[0] == 0x42);
}

static void test_back_to_back_frames(void)
{
    const uint8_t a[2] = {7, 8}, b[1] = {9};
    uint8_t stream[5 + 4];
    pkt_parser_t p;
    size_t i, na, nb, frames = 0;
    pkt_status_t s;
    na = pkt_build(stream, sizeof stream, a, 2);
    nb = pkt_build(stream + na, sizeof stream - na, b, 1);
    pkt_init(&p);
    for (i = 0; i < na + nb; i++) {
        s = pkt_feed(&p, stream[i]);
        if (s == PKT_FRAME_READY) {
            frames++;
        }
    }
    CHECK("PKT-006", frames == 2);
}

static void test_builder_and_null_handling(void)
{
    uint8_t out[4];
    const uint8_t pl[1] = {5};
    CHECK("PKT-007", pkt_build(NULL, 4, pl, 1) == 0);
    CHECK("PKT-007", pkt_build(out, 4, NULL, 1) == 0);
    CHECK("PKT-007", pkt_build(out, 4, pl, PKT_MAX_PAYLOAD + 1) == 0);
    CHECK("PKT-007", pkt_build(out, 3, pl, 1) == 0); /* buffer too small */
    CHECK("PKT-007", pkt_feed(NULL, 0) == PKT_ERR_NULL);
    pkt_init(NULL); /* must not crash */
    CHECK("PKT-007", pkt_checksum(1, NULL) == 0xFF);
}

int main(void)
{
    test_valid_frame();
    test_empty_and_max_payload();
    test_checksum_error_and_recovery();
    test_oversize_length();
    test_garbage_before_start();
    test_back_to_back_frames();
    test_builder_and_null_handling();
    printf("%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
