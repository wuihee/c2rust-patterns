static curl_socket_t mqttit(curl_socket_t fd) {
  // [Capacity]
  size_t buff_size = 10 * 1024;
  // [Vec Pointer]
  unsigned char *buffer = NULL;
  ssize_t rc;
  unsigned char byte;
  unsigned short packet_id;
  size_t payload_len;
  size_t client_id_length;
  size_t topic_len;
  // [Length]
  size_t remaining_length = 0;
  size_t bytes = 0; /* remaining length field size in bytes */
  char client_id[MAX_CLIENT_ID_LENGTH];
  long testno;
  FILE *stream = NULL;
  FILE *dump;
  char dumpfile[256];

  static const char protocol[7] = {
      0x00, 0x04,           /* protocol length */
      'M',  'Q',  'T', 'T', /* protocol name */
      0x04                  /* protocol level */
  };
  snprintf(dumpfile, sizeof(dumpfile), "%s/%s", logdir, REQUEST_DUMP);
  dump = curlx_fopen(dumpfile, "ab");
  if (!dump)
    goto end;

  mqttd_getconfig();

  testno = m_config.testnum;

  if (testno)
    logmsg("Found test number %ld", testno);

  buffer = malloc(buff_size);
  if (!buffer) {
    logmsg("Out of memory, unable to allocate buffer");
    goto end;
  }
  memset(buffer, 0, buff_size);

  do {
    unsigned char usr_flag = 0x80;
    unsigned char passwd_flag = 0x40;
    unsigned char conn_flags;
    const size_t client_id_offset = 12;
    size_t start_usr;
    size_t start_passwd;

    /* get the fixed header */
    rc = fixedheader(fd, &byte, &remaining_length, &bytes);
    if (rc)
      break;

    // [Ensure Capacity]
    if (remaining_length >= buff_size) {
      unsigned char *newbuffer;
      buff_size = remaining_length;
      newbuffer = realloc(buffer, buff_size);
      if (!newbuffer) {
        logmsg("Failed realloc of size %zu", buff_size);
        goto end;
      }
      buffer = newbuffer;
    }

    if (remaining_length) {
      /* reading variable header and payload into buffer */
      // [Push Value: Invalid]
      rc = sread(fd, buffer, remaining_length);
      if (rc > 0) {
        logmsg("READ %zd bytes", rc);
        loghex(buffer, rc);
      }
    }

    if (byte == MQTT_MSG_CONNECT) {
      logprotocol(FROM_CLIENT, "CONNECT", remaining_length, dump, buffer, rc);

      if (memcmp(protocol, buffer, sizeof(protocol))) {
        logmsg("Protocol preamble mismatch");
        goto end;
      }
      /* ignore the connect flag byte and two keepalive bytes */
      payload_len = (size_t)(buffer[10] << 8) | buffer[11];
      /* first part of the payload is the client ID */
      client_id_length = payload_len;

      /* checking if user and password flags were set */
      conn_flags = buffer[7];

      start_usr = client_id_offset + payload_len;
      if (usr_flag == (conn_flags & usr_flag)) {
        logmsg("User flag is present in CONN flag");
        payload_len += (size_t)(buffer[start_usr] << 8) | buffer[start_usr + 1];
        payload_len += 2; /* MSB and LSB for user length */
      }

      start_passwd = client_id_offset + payload_len;
      if (passwd_flag == (conn_flags & passwd_flag)) {
        logmsg("Password flag is present in CONN flags");
        payload_len +=
            (size_t)(buffer[start_passwd] << 8) | buffer[start_passwd + 1];
        payload_len += 2; /* MSB and LSB for password length */
      }

      /* check the length of the payload */
      if ((ssize_t)payload_len != (rc - 12)) {
        logmsg("Payload length mismatch, expected %zx got %zx", rc - 12,
               payload_len);
        goto end;
      }
      /* check the length of the client ID */
      else if ((client_id_length + 1) > MAX_CLIENT_ID_LENGTH) {
        logmsg("Too large client id");
        goto end;
      }
      memcpy(client_id, &buffer[12], client_id_length);
      client_id[client_id_length] = 0;

      logmsg("MQTT client connect accepted: %s", client_id);

      /* The first packet sent from the Server to the Client MUST be a
         CONNACK Packet */

      if (connack(dump, fd)) {
        logmsg("failed sending CONNACK");
        goto end;
      }
    } else if (byte == MQTT_MSG_SUBSCRIBE) {
      int error;
      char *data;
      size_t datalen;
      logprotocol(FROM_CLIENT, "SUBSCRIBE", remaining_length, dump, buffer, rc);
      logmsg("Incoming SUBSCRIBE");

      if (rc < 6) {
        logmsg("Too small SUBSCRIBE");
        goto end;
      }

      /* two bytes packet id */
      packet_id = (unsigned short)((buffer[0] << 8) | buffer[1]);

      /* two bytes topic length */
      topic_len = (size_t)(buffer[2] << 8) | buffer[3];
      if (topic_len != (remaining_length - 5)) {
        logmsg("Wrong topic length, got %zu expected %zu", topic_len,
               remaining_length - 5);
        goto end;
      }
      memcpy(topic, &buffer[4], topic_len);
      topic[topic_len] = 0;

      /* there is a QoS byte (two bits) after the topic */

      logmsg("SUBSCRIBE to '%s' [%d]", topic, packet_id);
      stream = test2fopen(testno, logdir);
      if (!stream) {
        char errbuf[STRERROR_LEN];
        error = errno;
        logmsg("fopen() failed with error (%d) %s", error,
               curlx_strerror(error, errbuf, sizeof(errbuf)));
        logmsg("Could not open test file %ld", testno);
        goto end;
      }
      error = getpart(&data, &datalen, "reply", "data", stream);
      if (!error) {
        if (!m_config.publish_before_suback) {
          if (suback(dump, fd, packet_id)) {
            logmsg("failed sending SUBACK");
            free(data);
            goto end;
          }
        }
        if (publish(dump, fd, packet_id, topic, data, datalen)) {
          logmsg("PUBLISH failed");
          free(data);
          goto end;
        }
        free(data);
        if (m_config.publish_before_suback) {
          if (suback(dump, fd, packet_id)) {
            logmsg("failed sending SUBACK");
            goto end;
          }
        }
      } else {
        const char *def = "this is random payload yes yes it is";
        publish(dump, fd, packet_id, topic, def, strlen(def));
      }
      disconnect(dump, fd);
    } else if ((byte & 0xf0) == (MQTT_MSG_PUBLISH & 0xf0)) {
      size_t topiclen;

      logmsg("Incoming PUBLISH");
      logprotocol(FROM_CLIENT, "PUBLISH", remaining_length, dump, buffer, rc);

      topiclen = (size_t)(buffer[1 + bytes] << 8) | buffer[2 + bytes];
      logmsg("Got %zu bytes topic", topiclen);

#ifdef QOS
      /* Handle packetid if there is one. Send puback if QoS > 0 */
      puback(dump, fd, 0);
#endif
      /* expect a disconnect here */
      /* get the request */
      rc = sread(fd, &buffer[0], 2);

      logmsg("READ %zd bytes [DISCONNECT]", rc);
      loghex(buffer, rc);
      logprotocol(FROM_CLIENT, "DISCONNECT", 0, dump, buffer, rc);
      goto end;
    } else {
      /* not supported (yet) */
      goto end;
    }
  } while (1);

end:
  if (buffer)
    free(buffer);
  if (dump)
    curlx_fclose(dump);
  if (stream)
    curlx_fclose(stream);
  return CURL_SOCKET_BAD;
}
