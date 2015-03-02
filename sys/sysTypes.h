/*
 * sysTypes.h
 *
 *  Created on: Feb 23, 2015
 *      Author: titan
 */

#ifndef SYSTYPES_H_
#define SYSTYPES_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define PRAGMA(x)

#define PACK __attribute__ ((packed))

#define INLINE static inline __attribute__ ((always_inline))

#define SYS_EnableInterrupts() sei()

#define ATOMIC_SECTION_ENTER   { uint8_t __atomic = SREG; cli();
#define ATOMIC_SECTION_LEAVE   SREG = __atomic; }

#endif /* SYSTYPES_H_ */
