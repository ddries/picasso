#include "chunk.h"
#include "crc.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

chunk_t* create_chunk(void)
{
    chunk_t *c = malloc(sizeof(chunk_t));
    c->type = 0;
    c->length = 0;
    c->crc = 0;
    c->data = NULL;
    return c;
}

void init_chunk(chunk_t *chunk)
{
    assert(chunk->length > 0);
    chunk->data = malloc(chunk->length);
}

void free_chunk(chunk_t *chunk)
{
    chunk->type = 0;
    chunk->length = 0;
    chunk->crc = 0;
    free(chunk->data);
}

void crc_chunk(chunk_t *chunk)
{
    uint64_t total_length = sizeof(chunk->length) + chunk->length;
    uint8_t data[total_length];

    memcpy(data, &chunk->type, sizeof(chunk->type));
    memcpy(data + sizeof(chunk->type), chunk->data, chunk->length);

    printf("total length: %lu\n", total_length);
    for (size_t i = 0; i < total_length; i++)
    {
        printf("%x ", *(data + i));
    }
    printf("\n");

    uint32_t c = update_crc(0xffffffffL, data, total_length) ^ 0xffffffffL;
    chunk->crc = c;
}

void print_chunk(chunk_t *chunk)
{
    printf("== Chunk ==\n");
    printf("Type: %u (%04x)\n", chunk->type, chunk->type);
    printf("Length: %u\n", chunk->length);
    printf("CRC: %u (%x)\n", chunk->crc, chunk->crc);
    printf("Data:\n");

    for (uint32_t i = 0; i < chunk->length; i++)
    {
        printf("%x ", *(chunk->data + i));
    }

    printf("\n== Chunk ==\n");
}

void write_bytes_to_chunk(chunk_t *chunk, uint8_t *bytes, size_t count)
{
    for (size_t i = 0; i < count; i++)
        *(chunk->data + i) = *bytes++;
}