#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "chunk.h"

#define get_nth_byte(bytes, nth) (bytes >> (nth * 8)) & 0xff
#define build_u32(b3, b2, b1, b0) (b3 << 24 | b2 << 16 | b1 << 8 | b0)

typedef struct {
    size_t capacity;
    size_t position;
    uint8_t *data;
} buffer_t;

#define write_byte_or_die(buffer, byte) __write_byte_or_die(buffer, byte, __FILE__, __LINE__);
void __write_byte_or_die(buffer_t *buffer, uint8_t byte, const char* source_file, int source_line)
{
    if (buffer->position >= buffer->capacity)
    {
        fprintf(stderr, "(%s:%d) ERROR: Buffer overflow, tried: %zu, max: %zu\n",
            source_file, source_line, buffer->position, buffer->capacity);
        exit(1);
    }

    *(buffer->data + buffer->position++) = byte;
}

#define file_write_byte_or_die(file, byte) __file_write_byte_or_die(file, byte, __FILE__, __LINE__);
void __file_write_byte_or_die(FILE *file, uint8_t byte, const char* source_file, int source_line)
{
    size_t n = fwrite(&byte, sizeof(byte), 1, file);

    if (n != 1)
    {
        assert(ferror(file) != 0);
        fprintf(stderr, "(%s:%d) ERROR: Could not write (%x) to file: %s\n",
            source_file, source_line, byte, strerror(errno));
        exit(1);
    }
}

#define write_bytes_or_die(buffer, bytes, count) __write_bytes_or_die(buffer, bytes, count, __FILE__, __LINE__);
void __write_bytes_or_die(buffer_t *buffer, void *bytes0, size_t count, const char* source_file, int source_line)
{
    uint8_t *bytes = (uint8_t*)bytes0;
    for (size_t i = 0; i < count; i++)
        __write_byte_or_die(buffer, *(bytes + i), source_file, source_line);
}

#define file_write_bytes_or_die(file, bytes, count) __file_write_bytes_or_die(file, bytes, count, __FILE__, __LINE__);
void __file_write_bytes_or_die(FILE *file, void *bytes0, size_t count, const char* source_file, int source_line)
{
    uint8_t *bytes = (uint8_t*)bytes0;
    for (size_t i = 0; i < count; i++)
        __file_write_byte_or_die(file, *(bytes + i), source_file, source_line);
}

// #define write_octet_or_die(buffer, byte) __write_octet_or_die(buffer, byte, __FILE__, __LINE__);
// void __write_octet_or_die(buffer_t *buffer, uint32_t octet, const char* source_file, int source_line)
// {
//     for (uint8_t i = 0; i < sizeof(octet); i++)
//         __write_byte_or_die(buffer, get_nth_byte(octet, i), source_file, source_line);
// }

void print_bytes(void *bytes0, size_t count)
{
    uint8_t *bytes = (uint8_t*) bytes0;
    for (size_t i = 0; i < count; i++)
        printf("%x ", *(bytes + i));
    printf("\n");
}

void reverse_bytes(uint32_t *a)
{
    uint32_t temp, i;
    for (i = 0, temp = 0; i < 4; i++)
        temp |= (get_nth_byte(*a, i)) << ((4 - i - 1) * 8);
    *a = temp;
}

typedef struct {
    uint32_t width;
    uint32_t height;
} image_size_u;

image_size_u compute_image_size(uint32_t pixel_count)
{
    static const float resolution = 4/3;
    image_size_u size;

    size.width = ceil(sqrtf(resolution * pixel_count));
    size.height = (1 / resolution) * size.width;

    return size;
}

size_t get_bytes_from_chunk(chunk_t *chunk, uint8_t* buffer)
{
    size_t chunk_sz = chunk->length;
    size_t offset = 0;

    reverse_bytes(&chunk->length);
    reverse_bytes(&chunk->type);
    reverse_bytes(&chunk->crc);

    for (size_t i = 0; i < sizeof(chunk->length); i++)
        *(buffer + i) = get_nth_byte(chunk->length, i);
    offset += sizeof(chunk->length);

    for (size_t i = 0; i < sizeof(chunk->type); i++)
        *(buffer + offset + i) = get_nth_byte(chunk->type, i);
    offset += sizeof(chunk->type);
    
    for (size_t i = 0; i < chunk_sz; i++)
        *(buffer + offset + i) = *(chunk->data + i);
    offset += chunk_sz;

    for (size_t i = 0; i < sizeof(chunk->crc); i++)
        *(buffer + offset + i) = get_nth_byte(chunk->crc, i);
    offset += sizeof(chunk->crc);

    return offset;
}

#define BUF_SIZE 16 * 1024 * 1024
static uint8_t png_sig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

int main(int argc, char** argv)
{
    (void) argc;
    assert(*argv != NULL);

    char *program = *argv++;

    if (*argv == NULL)
    {
        fprintf(stderr, "ERROR: No input file provided\n");
        printf("Usage: %s <input file> <output>\n", program);
        exit(1);
    }

    char *input_filepath = *argv++;
    char *output_filepath = "output.png";

    printf("Requested file: %s ...... ", input_filepath);
    
    FILE *input_file = fopen(input_filepath, "rb");

    if (input_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open input file (%s)\n", input_filepath);
        exit(1);
    }

    printf("OK\n");
    printf("Output file: %s\n", output_filepath);

    fseek(input_file, 0L, SEEK_END);
    size_t input_file_sz = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);

    printf("Input file size: %zu bytes\n", input_file_sz);

    FILE *output_file = fopen(output_filepath, "wb");

    if (output_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open output file (%s)\n", output_filepath);
        exit(1);
    }

    image_size_u image_size = compute_image_size(input_file_sz / 4);
    printf("Image size: %ux%u\n", image_size.width, image_size.height);

    chunk_t *chunk = create_chunk();
    chunk->length = 13;
    chunk->type = build_u32('I', 'H', 'D', 'R');
    init_chunk(chunk);

    buffer_t *buf = malloc(sizeof(buffer_t));
    buf->data = malloc(BUF_SIZE);
    buf->capacity = BUF_SIZE;

    reverse_bytes(&image_size.height);
    reverse_bytes(&image_size.width);

    write_bytes_or_die(buf, &image_size.width, sizeof(image_size.width));
    write_bytes_or_die(buf, &image_size.height, sizeof(image_size.height));
    __write_bytes_or_die(buf, (uint8_t[5]){ 16, 6, 0, 0, 0 }, 5, __FILE__, __LINE__);

    reverse_bytes(&image_size.height);
    reverse_bytes(&image_size.width);

    write_bytes_to_chunk(chunk, buf->data, chunk->length);

    reverse_bytes(&chunk->type);
    crc_chunk(chunk);
    reverse_bytes(&chunk->type);
    
    print_chunk(chunk);

    file_write_bytes_or_die(output_file, png_sig, 8);

    size_t n = get_bytes_from_chunk(chunk, buf->data);
    file_write_bytes_or_die(output_file, buf->data, n);
    
    fclose(input_file);
    fclose(output_file);

}