#include <libc/libc.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>

namespace dark::libc::__details {

static void throw_not_implemented() {
    throw FailToInterpret { Error::NotImplemented };
}

void puts(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void putchar(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void printf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void sprintf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void getchar(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void scanf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void sscanf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void malloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void calloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void realloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void free(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memset(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memcmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memcpy(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memmove(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcpy(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strlen(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcat(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strncmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

} // namespace dark::libc
