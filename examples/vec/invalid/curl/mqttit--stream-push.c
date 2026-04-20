// repos/curl/tests/server/mqttd.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef int curl_socket_t;
typedef ssize_t (*sread_fn)(curl_socket_t, void *, size_t);
static sread_fn sread;

static curl_socket_t mqttit(curl_socket_t fd) {
  // [Capacity]
  size_t buff_size = 10 * 1024;

  // [Pointer]
  unsigned char *buffer = malloc(buff_size);

  // [Length] — remaining_length is the packet payload length, used as length
  // of data written into the buffer each iteration.
  size_t remaining_length = 0;

  // ... (MQTT fixed-header parsing sets remaining_length each iteration)

  for (;;) {
    // parse fixed header into remaining_length ...

    // [Ensure Capacity]
    if (remaining_length >= buff_size) {
      buff_size = remaining_length;
      unsigned char *newbuffer = realloc(buffer, buff_size);
      if (!newbuffer)
        goto end;
      buffer = newbuffer;
    }

    // [Push Value: Stream] — reads from a socket fd directly into the buffer
    // instead of copying from another in-memory buffer.
    sread(fd, buffer, remaining_length);

    // ... (protocol dispatch on buffer contents)
  }

end:
  free(buffer);
  return -1;
}
