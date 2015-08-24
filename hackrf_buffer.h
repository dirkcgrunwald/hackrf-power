#ifndef hackrf_buffer_h
#define hackrf_buffer_h
/*
 * Allocate buffer, use zero for default size
 */

void hackrf_buffer_init(int size, int verbose);
int  hackrf_rx_callback(hackrf_transfer* transfer);
void hackrf_buffer_reset();
void hackrf_read_sync(uint8_t *buffer, int bufsize, int *bytesread);

#endif
