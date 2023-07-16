TARGET = picasso
SOURCES = picasso.c crc.c chunk.c
CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -Lexternal/zlib -lz -lm

picasso: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

zlib:
	./build_zlib.sh

clean:
	rm -r $(TARGET)
