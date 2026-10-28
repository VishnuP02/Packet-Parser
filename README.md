# Packet-Parser

A byte-at-a-time frame parser (state machine) in C99 with no dynamic allocation, verified with requirements-based tests and structural coverage analysis.

Frame layout: `[0x7E] [LEN] [PAYLOAD x LEN] [CHECKSUM]`, where `LEN` is 0 to 16 and the checksum makes the sum of `LEN`, the payload, and the checksum equal 0 mod 256.

## Requirements

| ID | Requirement |
|----|-------------|
| PKT-001 | The parser shall report a frame as ready, with the payload intact, when a valid frame is received. |
| PKT-002 | The parser shall accept zero-length and maximum-length (16-byte) payloads. |
| PKT-003 | The parser shall report a checksum error for a corrupted frame and shall parse the next valid frame correctly. |
| PKT-004 | The parser shall reject a length field above the maximum and return to waiting for a start byte. |
| PKT-005 | The parser shall ignore bytes received before a start byte. |
| PKT-006 | The parser shall parse back-to-back frames without losing any. |
| PKT-007 | The frame builder and parser shall reject NULL pointers, oversize lengths, and undersized output buffers without crashing. |

Every check in `test_packet.c` is tagged with the requirement it verifies.

## Build and test

    make test        # build with -Wall -Wextra -Wpedantic -Werror and run the tests
    make coverage    # rebuild with gcov instrumentation and print line/branch coverage

## Results

- 19 checks, 0 failures
- gcov: 100% of lines (52) and 100% of branches (34) in `packet.c`
