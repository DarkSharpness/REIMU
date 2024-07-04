#include <libc/libc.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>
#include <interpreter/executable.h>

namespace dark::libc::details {

void puts(Executable &, RegisterFile &, Memory &, Device &) {}
void putchar(Executable &, RegisterFile &, Memory &, Device &) {}
void printf(Executable &, RegisterFile &, Memory &, Device &) {}
void sprintf(Executable &, RegisterFile &, Memory &, Device &) {}
void getchar(Executable &, RegisterFile &, Memory &, Device &) {}
void scanf(Executable &, RegisterFile &, Memory &, Device &) {}
void sscanf(Executable &, RegisterFile &, Memory &, Device &) {}

void malloc(Executable &, RegisterFile &, Memory &, Device &) {}
void calloc(Executable &, RegisterFile &, Memory &, Device &) {}
void realloc(Executable &, RegisterFile &, Memory &, Device &) {}
void free(Executable &, RegisterFile &, Memory &, Device &) {}

void memset(Executable &, RegisterFile &, Memory &, Device &) {}
void memcmp(Executable &, RegisterFile &, Memory &, Device &) {}
void memcpy(Executable &, RegisterFile &, Memory &, Device &) {}
void memmove(Executable &, RegisterFile &, Memory &, Device &) {}

void strcpy(Executable &, RegisterFile &, Memory &, Device &) {}
void strlen(Executable &, RegisterFile &, Memory &, Device &) {}
void strcat(Executable &, RegisterFile &, Memory &, Device &) {}
void strcmp(Executable &, RegisterFile &, Memory &, Device &) {}
void strncmp(Executable &, RegisterFile &, Memory &, Device &) {}

} // namespace dark::libc
