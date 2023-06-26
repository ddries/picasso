#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define get_nth_byte(bytes, nth) (bytes >> (nth * 8)) & 0xff
#define build_u32(b3, b2, b1, b0) (b3 << 24 | b2 << 16 | b1 << 8 | b0)

typedef struct {
    uint8_t *data;
    size_t position;
    size_t capacity;
} buffer_t;

#define _write_byte_or_die(buffer, byte) __write_byte_or_die(buffer, byte, __FILE__, __LINE__);
void __write_byte_or_die(buffer_t *buffer, uint8_t byte, const char* source_file, int source_line)
{
    if (buffer->position >= buffer->capacity)
    {
        fprintf(stderr, "(%s:%d) ERROR: Buffer overflow, tried: %zu, max: %zu\n",
            source_file, source_line, buffer->position, buffer->capacity);
        exit(1);
    }

    *(buffer->data + buffer->position) = byte;
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
void __write_bytes_or_die(buffer_t *buffer, uint8_t* bytes, size_t count, const char* source_file, int source_line)
{
    for (uint8_t i = 0; i < count; i++)
        __write_byte_or_die(buffer, *(bytes + i), source_file, source_line);
}

#define file_write_bytes_or_die(file, bytes, count) __file_write_bytes_or_die(file, bytes, count, __FILE__, __LINE__);
void __file_write_bytes_or_die(FILE *file, uint8_t* bytes, size_t count, const char* source_file, int source_line)
{
    for (uint8_t i = 0; i < count; i++)
        __file_write_byte_or_die(file, *(bytes + i), source_file, source_line);
}

// #define write_octet_or_die(buffer, byte) __write_octet_or_die(buffer, byte, __FILE__, __LINE__);
// void __write_octet_or_die(buffer_t *buffer, uint32_t octet, const char* source_file, int source_line)
// {
//     for (uint8_t i = 0; i < sizeof(octet); i++)
//         __write_byte_or_die(buffer, get_nth_byte(octet, i), source_file, source_line);
// }

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

    printf("Input file size: %lu bytes\n", input_file_sz);

    FILE *output_file = fopen(output_filepath, "wb");

    if (output_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open output file (%s)\n", output_filepath);
        exit(1);
    }

    file_write_bytes_or_die(output_file, png_sig, 8);
    
    fclose(input_file);
    fclose(output_file);
    exit(0);

}