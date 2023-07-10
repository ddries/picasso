#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "chunk.h"
#include "external/zlib/zlib.h"

/* get nth byte from u32 or whatever */
#define get_nth_byte(bytes, nth) (bytes >> (nth * 8)) & 0xff

/* build u32 from given bytes */
#define build_u32(b3, b2, b1, b0) (b3 << 24 | b2 << 16 | b1 << 8 | b0)

/* iterable buffer */
typedef struct {
    size_t capacity;
    size_t position;
    uint8_t *data;
} buffer_t;

/* write single byte to buffer */
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

/* write single byte to file */
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

/* write multiple bytes to buffer */
#define write_bytes_or_die(buffer, bytes, count) __write_bytes_or_die(buffer, bytes, count, __FILE__, __LINE__);
void __write_bytes_or_die(buffer_t *buffer, void *bytes0, size_t count, const char* source_file, int source_line)
{
    uint8_t *bytes = (uint8_t*)bytes0;
    for (size_t i = 0; i < count; i++)
        __write_byte_or_die(buffer, *(bytes + i), source_file, source_line);
}

/* write multiple bytes to file */
#define file_write_bytes_or_die(file, bytes, count) __file_write_bytes_or_die(file, bytes, count, __FILE__, __LINE__);
void __file_write_bytes_or_die(FILE *file, void *bytes0, size_t count, const char* source_file, int source_line)
{
    uint8_t *bytes = (uint8_t*)bytes0;
    size_t n = fwrite(bytes, sizeof(uint8_t), count, file);

    if (n != count)
    {
        assert(ferror(file) != 0);
        fprintf(stderr, "(%s:%d) ERROR: Could not write %zu bytes to file: %s\n",
            source_file, source_line, count, strerror(errno));
        exit(1);
    }
    // for (size_t i = 0; i < count; i++)
    //     __file_write_byte_or_die(file, *(bytes + i), source_file, source_line);
}

/* read multiple bytes from file, return number of bytes read */
#define file_read_bytes_or_die(file, bytes, count) __file_read_bytes_or_die(file, bytes, count, __FILE__, __LINE__);
size_t __file_read_bytes_or_die(FILE *file, void *buf0, size_t count, const char* source_file, int source_line)
{
    uint8_t *buf = (uint8_t*)buf0;
    size_t pos = ftell(file);
    size_t n = fread(buf, 1, count, file);

    if (n != 1)
    {
        if (ferror(file) != 0)
        {
            fprintf(stderr, "(%s:%d) ERROR: Could not read %zu bytes from file: %s\n",
                source_file, source_line, count, strerror(errno));
            exit(1);
        } else if (feof(file) != 0)
        {
            if (ftell(file) - pos <= 0)
            {
                return 0;
            }

            fprintf(stderr, "(%s:%d) ERROR: Could not read %zu bytes from file: Reached EOF\n",
                source_file, source_line, count);

            fseek(file, pos, SEEK_SET);

            size_t i = 0;
            while (1)
            {
                n = fread(buf, 1, 1, file);
                // printf("read one byte\n");
                if (n == 0) break;
                i++;
            }
            printf("INFO: Read %zu remaining bytes until EOF\n", i);
            return i;
        }
    }

    return count;
}

/* print n bytes */
void print_bytes(void *bytes0, size_t count)
{
    uint8_t *bytes = (uint8_t*) bytes0;
    for (size_t i = 0; i < count; i++)
        printf("%x ", *(bytes + i));
    printf("\n");
}

/* reverse u32 bytes (le <-> be) */
void reverse_bytes(uint32_t *a)
{
    uint32_t temp, i;
    for (i = 0, temp = 0; i < 4; i++)
        temp |= (get_nth_byte(*a, i)) << ((4 - i - 1) * 8);
    *a = temp;
}

/* width and height of image */
typedef struct {
    uint32_t width;
    uint32_t height;
} image_size_t;

/* compute image size from pixel count */
image_size_t compute_image_size(uint32_t pixel_count)
{
    static const float resolution = 4/3;
    image_size_t size;

    size.width = ceil(sqrtf(resolution * pixel_count));
    size.height = (1 / resolution) * size.width;

    return size;
}

/* serialize chunk into byte array, save results into buffer and returns byte count */
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

/* buffer size */
#define BUF_SIZE 16 * 1024 * 1024

/* max size of chunk data section length */
#define MAX_CHUNK_LENGTH 2147483648

/* png signature */
static uint8_t png_sig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

int main(int argc, char** argv)
{
    (void) argc;
    assert(*argv != NULL);

    char *program = *argv++;

    if (*argv == NULL)
    {
        fprintf(stderr, "ERROR: No input file provided\n");
        printf("Usage: %s <input file>\n", program);
        exit(1);
    }

    char *input_filepath = *argv++;
    char *output_filepath = "output.png";

    printf("Requested file: %s", input_filepath);
    
    FILE *input_file = fopen(input_filepath, "rb");

    if (input_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open input file (%s)\n", input_filepath);
        exit(1);
    }

    printf("\tOK\n");

    fseek(input_file, 0L, SEEK_END);
    size_t input_file_sz = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);

    printf("Input file size: %zu bytes\n", input_file_sz);
    printf("Output file: %s", output_filepath);

    FILE *output_file = fopen(output_filepath, "wb");

    if (output_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open output file (%s)\n", output_filepath);
        exit(1);
    }

    printf("\t\tOK\n");

    /* pixel count is byte count /4 (using rgba) */
    image_size_t image_size = compute_image_size(input_file_sz / 4);
    printf("Image size: %ux%u\n", image_size.width, image_size.height);

    buffer_t *buf = malloc(sizeof(buffer_t));
    buf->data = malloc(BUF_SIZE);
    buf->capacity = BUF_SIZE;

    chunk_t *chunk = create_chunk();
    
    /* IHDR chunk */

    chunk->length = 13;
    chunk->type = build_u32('I', 'H', 'D', 'R');

    init_chunk(chunk);

    reverse_bytes(&image_size.height);
    reverse_bytes(&image_size.width);

    write_bytes_or_die(buf, &image_size.width, sizeof(image_size.width));
    write_bytes_or_die(buf, &image_size.height, sizeof(image_size.height));

    reverse_bytes(&image_size.height);
    reverse_bytes(&image_size.width);

    /* color depth: 8 bit, color type: 6 (rgba), compression method: 0 (deflate), filter method: 0 (adaptive), interlace method: 0 (no interlace) */
    __write_bytes_or_die(buf, (uint8_t[5]){ 8, 6, 0, 0, 0 }, 5, __FILE__, __LINE__);

    write_bytes_to_chunk(chunk, buf->data, chunk->length);

    reverse_bytes(&chunk->type);
    crc_chunk(chunk);

    file_write_bytes_or_die(output_file, png_sig, 8);

    size_t n = get_bytes_from_chunk(chunk, buf->data);
    file_write_bytes_or_die(output_file, buf->data, n);
    free_chunk(chunk);


    /* IDAT chunk */
    chunk->type = build_u32('I', 'D', 'A', 'T');

    /* reset buffer */
    buf->position = 0;

    /* 1. filtering */
    printf("Reading scanlines from %s\n", input_filepath);
    printf("Scanline size: %d, Scanline count: %d\n", image_size.width * 4, image_size.height);
    
    uint32_t bytes_per_scanline = image_size.width * 4;
    uint8_t *bytes[bytes_per_scanline];

    for (size_t i = 0, n = 0; i < image_size.height; i++) // iterate scanlines
    {
        printf("Reading from byte %lu to %lu", bytes_per_scanline * i, bytes_per_scanline * i + bytes_per_scanline - 1);
        n = file_read_bytes_or_die(input_file, bytes, bytes_per_scanline);

        printf("\tOK\n");
        // printf("Read bytes: ");
        // print_bytes(bytes, n);
        write_byte_or_die(buf, 0); // None filter
        write_bytes_or_die(buf, bytes, n);
        // printf("Scanline (with filtering): ");
        // print_bytes(bytes, n + 1);

        if (n < bytes_per_scanline)
        {
            break;
        }
    }

    // printf("Buffer after filtering:\n");
    // print_bytes(buf->data, bytes_per_scanline * image_size.height);

    /* 2. compression */
    z_stream zlib_stream;
    memset(&zlib_stream, 0, sizeof(zlib_stream));

    printf("Initializing zlib...");

    if (deflateInit(&zlib_stream, Z_DEFAULT_COMPRESSION) != Z_OK)
    {
        fprintf(stderr, "ERROR: Could not initialize deflate stream\n");
        exit(1);
    }

    uint8_t *zlib_buffer = malloc(BUF_SIZE);
    zlib_stream.next_in = (Bytef*)buf->data;
    zlib_stream.next_out = (Bytef*)zlib_buffer;
    zlib_stream.avail_in = (uInt)buf->position + 1;
    zlib_stream.avail_out = (uInt)BUF_SIZE;

    printf("\t\tOK\n");
    printf("Compressing... ");

    if (deflate(&zlib_stream, Z_FINISH) != Z_STREAM_END)
    {
        fprintf(stderr, "ERROR: Could not compress data\n");
        exit(1);
    }

    deflateEnd(&zlib_stream);

    printf("\t\t\tOK\n");

    size_t compressed_sz = BUF_SIZE - zlib_stream.avail_out;
    chunk->length = compressed_sz;

    printf("Compressed data size: %zu bytes\n", compressed_sz);

    printf("Write bytes to chunk...");
    write_bytes_to_chunk(chunk, zlib_buffer, compressed_sz);

    reverse_bytes(&chunk->type);
    crc_chunk(chunk);
    printf("\t\tOK\n");

    n = get_bytes_from_chunk(chunk, buf->data);
    printf("Chunk size: %zu bytes\n", n);

    file_write_bytes_or_die(output_file, buf->data, n);
    free_chunk(chunk);

    /* IEND chunk */

    chunk->length = 0;
    chunk->type = build_u32('I', 'E', 'N', 'D');
    crc_chunk(chunk);

    n = get_bytes_from_chunk(chunk, buf->data);
    file_write_bytes_or_die(output_file, buf->data, n);
    
    fclose(input_file);
    fclose(output_file);
}