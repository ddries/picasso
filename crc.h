/* CRC sample code taken from PNG spec: http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html */

/* Make the table for a fast CRC. */
void make_crc_table(void);

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
    should be initialized to all 1's, and the transmitted value
    is the 1's complement of the final running CRC (see the
    crc() routine below)). */
unsigned long update_crc(unsigned long crc, unsigned char *buf, int len);

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len);