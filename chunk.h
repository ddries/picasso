#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint32_t length;
    uint32_t type;
    uint8_t *data;
    uint32_t crc;
} chunk_t;

chunk_t* create_chunk(void);
void init_chunk(chunk_t *chunk);
void free_chunk(chunk_t *chunk);
void crc_chunk(chunk_t *chunk);
void print_chunk(chunk_t *chunk);
void write_bytes_to_chunk(chunk_t *chunk, uint8_t *bytes, size_t count);