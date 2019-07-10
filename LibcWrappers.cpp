#include <cassert>
#include <cstdlib>
#include <cstring>

#include <sys/mman.h>
#include <unistd.h>

#include "Runtime.h"

#define SYM(x) __symbolized_##x

// #define DEBUG_RUNTIME
#ifdef DEBUG_RUNTIME
#include <iostream>
#endif

extern "C" {

void *SYM(malloc)(size_t size) {
  auto result = malloc(size);
  _sym_set_return_expression(_sym_build_integer(
      reinterpret_cast<uintptr_t>(result), sizeof(result) * 8));
  auto shadow = static_cast<Z3_ast *>(malloc(size * sizeof(Z3_ast)));
  _sym_register_memory(static_cast<uint8_t *>(result), shadow, size);
  return result;
}

void *SYM(mmap)(void *addr, size_t len, int prot, int flags, int fildes,
                off_t off) {
  auto result = mmap(addr, len, prot, flags, fildes, off);
  _sym_set_return_expression(_sym_build_integer(
      reinterpret_cast<uintptr_t>(result), sizeof(result) * 8));
  auto shadow = static_cast<Z3_ast *>(malloc(len * sizeof(Z3_ast)));
  _sym_register_memory(static_cast<uint8_t *>(result), shadow, len);
  return result;
}

char *SYM(getenv)(const char *name) {
#ifdef DEBUG_RUNTIME
  std::cout << "Intercepting call to getenv with argument " << name
            << std::endl;
#endif
  auto result = getenv(name);
  // TODO register string memory?
  _sym_set_return_expression(_sym_build_integer(
      reinterpret_cast<uintptr_t>(result), sizeof(result) * 8));
  return result;
}

ssize_t SYM(read)(int fildes, void *buf, size_t nbyte) {
  auto result = read(fildes, buf, nbyte);
  auto region = _sym_get_memory_region(buf);
  auto byteBuf = static_cast<uint8_t *>(buf);
  assert(region && (byteBuf + nbyte <= region->end) && "Unknown memory region");
  _sym_initialize_memory(byteBuf, region->shadow + (byteBuf - region->start),
                         nbyte);
  _sym_set_return_expression(_sym_build_integer(result, sizeof(result) * 8));
  return result;
}

void *SYM(memcpy)(void *dest, const void *src, size_t n) {
  auto result = memcpy(dest, src, n);
  _sym_memcpy(static_cast<uint8_t *>(dest), static_cast<const uint8_t *>(src),
              n);
  _sym_set_return_expression(_sym_build_integer(
      reinterpret_cast<uintptr_t>(result), sizeof(result) * 8));
  return result;
}

void *SYM(memset)(void *s, int c, size_t n) {
  auto result = memset(s, c, n);
  _sym_memset(static_cast<uint8_t *>(s), _sym_build_integer(c, sizeof(c) * 8),
              n);
  _sym_set_return_expression(_sym_build_integer(
      reinterpret_cast<uintptr_t>(result), sizeof(result) * 8));
  return result;
}

int SYM(select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                struct timeval *timeout) {
  auto result = select(nfds, readfds, writefds, exceptfds, timeout);
  _sym_set_return_expression(_sym_build_integer(result, sizeof(result) * 8));
  return result;
}

ssize_t SYM(write)(int fildes, const void *buf, size_t nbyte) {
  auto result = write(fildes, buf, nbyte);
  _sym_set_return_expression(_sym_build_integer(result, sizeof(result) * 8));
  return result;
}
}