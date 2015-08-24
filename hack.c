#include <libhackrf/hackrf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#define U64TOA_MAX_DIGIT (31)
typedef struct 
{
  char data[U64TOA_MAX_DIGIT+1];
} t_u64toa;


#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */
#define FREQ_ONE_MHZ (1000000ull)

static hackrf_device* device = NULL;

static char *stringrev(char *str)
{
  char *p1, *p2;

  if(! str || ! *str)
    return str;

  for(p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
    {
      *p1 ^= *p2;
      *p2 ^= *p1;
      *p1 ^= *p2;
    }
  return str;
}


char* u64toa(uint64_t val, t_u64toa* str)
{
#define BASE (10ull) /* Base10 by default */
  uint64_t sum;
  int pos;
  int digit;
  int max_len;
  char* res;

  sum = val;
  max_len = U64TOA_MAX_DIGIT;
  pos = 0;

  do
    {
      digit = (sum % BASE);
      str->data[pos] = digit + '0';
      pos++;

      sum /= BASE;
    }while( (sum>0) && (pos < max_len) );

  if( (pos == max_len) && (sum>0) )
    return NULL;

  str->data[pos] = '\0';
  res = stringrev(str->data);

  return res;
}

int rx_callback(hackrf_transfer* transfer) {
  int i;
  fprintf(stderr,"rx_callback %d bytes\n", transfer -> valid_length);
  fprintf(stderr,"samples: ");
  for (i = 0; i < 10; i++) {
    fprintf(stderr, " %d", (int) transfer -> buffer[i]);
  }
  fprintf(stderr,"\n");
  return 0;
}

main()
{
  int vga_gain = 20;
  int lna_gain = 8;
  int result;
  const char* serial_number = NULL;
  int sample_rate_hz = DEFAULT_SAMPLE_RATE_HZ;
  int i;
  unsigned long long freq_hz = 91500000;
  t_u64toa ascii_u64_data1;

  result = hackrf_init();
  if( result != HACKRF_SUCCESS ) {
    printf("hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
    return EXIT_FAILURE;
  }
  
  result = hackrf_open(&device);
  if( result != HACKRF_SUCCESS ) {
    printf("hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
    return EXIT_FAILURE;
  }


  printf("call hackrf_sample_rate_set(%u Hz/%.03f MHz)\n",
	 sample_rate_hz,((float)sample_rate_hz/(float)FREQ_ONE_MHZ));
  result = hackrf_set_sample_rate_manual(device, sample_rate_hz, 1);

  if( result != HACKRF_SUCCESS ) {
    printf("hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
    return EXIT_FAILURE;
  }

  int baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);

  printf("call hackrf_baseband_filter_bandwidth_set(%d Hz/%.03f MHz)\n",
	 baseband_filter_bw_hz, ((float)baseband_filter_bw_hz/(float)FREQ_ONE_MHZ));

  result = hackrf_set_baseband_filter_bandwidth(device, baseband_filter_bw_hz);

  if( result != HACKRF_SUCCESS ) {
    printf("hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n",
	   hackrf_error_name(result), result);
    return EXIT_FAILURE;
  }

  result = hackrf_set_vga_gain(device, vga_gain);
  result |= hackrf_set_lna_gain(device, lna_gain);
  result |= hackrf_start_rx(device, rx_callback, NULL);

  fprintf(stderr,"Result is %d\n", result);

  printf("call hackrf_set_freq(%s Hz/%.03f MHz)\n",
	 u64toa(freq_hz, &ascii_u64_data1),((double)freq_hz/(double)FREQ_ONE_MHZ) );
  result = hackrf_set_freq(device, freq_hz);
  if( result != HACKRF_SUCCESS ) {
    printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
    return EXIT_FAILURE;
  }

  for(i = 0; i < 30; i++) {
    fprintf(stderr,"wait...\n");
    sleep(1);
  }
}
