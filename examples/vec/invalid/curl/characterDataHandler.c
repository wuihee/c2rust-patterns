static void characterDataHandler(void *userData, const XML_Char *s, int len) {
  struct ParserStruct *state = (struct ParserStruct *)userData;
  struct MemoryStruct *mem = &state->characters;

  // [Capacity Check?]
  char *ptr = realloc(mem->memory, mem->size + (unsigned long)len + 1);
  if (!ptr) {
    /* Out of memory. */
    fprintf(stderr, "Not enough memory (realloc returned NULL).\n");
    state->ok = 0;
    return;
  }

  // [Push Value: Extend]
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), s, len);
  mem->size += (unsigned long)len;
  mem->memory[mem->size] = 0;
}
