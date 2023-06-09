#ifndef POS_DRIVERTOOLS_H
#define POS_DRIVERTOOLS_H

#include <_memlayout.h>
#include <type.hpp>

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned short wchar;


typedef unsigned long uintptr_t;

#define NULL 0

#define readb(addr) (*(volatile uint8 *)(addr))
#define readw(addr) (*(volatile uint16 *)(addr))
#define readd(addr) (*(volatile uint32 *)(addr))
#define readq(addr) (*(volatile uint64 *)(addr))

#define writeb(v, addr)                      \
    {                                        \
        (*(volatile uint8 *)(addr)) = (v); \
    }
#define writew(v, addr)                       \
    {                                         \
        (*(volatile uint16 *)(addr)) = (v); \
    }
#define writed(v, addr)                       \
    {                                         \
        (*(volatile uint32 *)(addr)) = (v); \
    }
#define writeq(v, addr)                       \
    {                                         \
        (*(volatile uint64 *)(addr)) = (v); \
    }

void set_bit(volatile uint32 *bits, uint32 mask, uint32 value);
void set_bit_offset(volatile uint32 *bits, uint32 mask, uint64 offset, uint32 value);
void set_gpio_bit(volatile uint32 *bits, uint64 offset, uint32 value);
uint32 get_bit(volatile uint32 *bits, uint32 mask, uint64 offset);
uint32 get_gpio_bit(volatile uint32 *bits, uint64 offset);

void* Kalloc();
void Kfree(void *addr);

void* memmove(void *dst, const void *src,unsigned long long size);
void Memmove(char *dst, char *src, uint64 count);

void dmacSemWait();
void dmacSemSignal();
void dmacSemInit();

void DebugInfo(const char *str); 
void DebugInfoI(const char *str,unsigned long long x); 

#endif
