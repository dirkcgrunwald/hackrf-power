#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libhackrf/hackrf.h>
#include <pthread.h>
#include <string.h>

#include "hackrf_buffer.h"

static pthread_mutex_t hackrfBufferLock;
static int hackrfBufferHead;
static int hackrfBufferTail;
static int hackrfBufferEnd;
#define HACKRF_BUFFER_SIZE (1024 * 1024 * 8)
static uint8_t *hackrfBuffer;

static int verbose = 0;

void hackrf_buffer_init(int size, int _verbose)
{
  if (size == 0) {
    size = 1024 * 1024 *8;
  }
  verbose = _verbose;

  hackrfBuffer = malloc(size);
  if ( !hackrfBuffer ) {
    fprintf(stderr,"Unable to allocate hackrfBuffer\n");
    exit(1);
  }
  hackrfBufferHead = 0;
  hackrfBufferTail = 0;
  hackrfBufferEnd = size;

  pthread_mutex_init(&hackrfBufferLock, NULL);
}

int hackrf_rx_callback(hackrf_transfer* transfer) {
  int i;
  if ( verbose ) {
    fprintf(stderr,"rx_callback %d bytes\n", transfer -> valid_length);
  }

  int overflow = 0;
  pthread_mutex_lock(&hackrfBufferLock);
  /*
   * If there isn't enough room, move things to the front
   */
  int inBuffer = hackrfBufferTail - hackrfBufferHead;
  int tailLeft = hackrfBufferEnd - hackrfBufferTail;
  if ( inBuffer + transfer -> valid_length > hackrfBufferEnd ) {
    /* reader not keeping up, signal overflow and reset buffer */
    overflow = 1;
    hackrfBufferHead = 0;
    hackrfBufferTail = 0;
  } else if (tailLeft < transfer -> valid_length) {
    memcpy(&hackrfBuffer[0], &hackrfBuffer[hackrfBufferHead], inBuffer);
    hackrfBufferHead = 0;
    hackrfBufferTail = inBuffer;
  }

  memcpy(&hackrfBuffer[hackrfBufferTail], 
	 transfer -> buffer, transfer -> valid_length);
  hackrfBufferTail += transfer -> valid_length;

  pthread_mutex_unlock(&hackrfBufferLock);

  if ( overflow && verbose ) {
    fprintf(stderr,"hackrf_buffer overflow!\n");
  }

  if ( verbose ) {
    fprintf(stderr,"Exit hackrf_rx_callback\n");
  }

  return 0; /* always be happy */
}

void
hackrf_buffer_reset()
{
  pthread_mutex_lock(&hackrfBufferLock);
  hackrfBufferHead = 0;
  hackrfBufferTail = 0;
  pthread_mutex_unlock(&hackrfBufferLock);
}

void hackrf_read_sync(uint8_t *buffer, int bufsize, int *bytesread)
{

  while (hackrfBufferTail - hackrfBufferHead < bufsize) {
    if ( verbose ) {
      fprintf(stderr,"wait for buffered data..\n");
    }
    usleep(1000);
  }

  pthread_mutex_lock(&hackrfBufferLock);
  /*
   * If there isn't enough room, move things to the front
   */
  int inBuffer = hackrfBufferTail - hackrfBufferHead;
  int tailLeft = hackrfBufferEnd - hackrfBufferTail;
  int toMove = bufsize;
  
  if (inBuffer < bufsize) {
    toMove = inBuffer;
  }
  if (toMove > 0) {
    memcpy(buffer, &hackrfBuffer[hackrfBufferHead], toMove);
    hackrfBufferHead += toMove;
  }
  pthread_mutex_unlock(&hackrfBufferLock);

  *bytesread = toMove;
}
