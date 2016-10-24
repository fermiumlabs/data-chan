/**
	data-chan physic through USB
	Copyright (C) 2016  Benato Denis
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MEASURE_H__
#define __MEASURE_H__

// Import USB configuration
#include "../config.h"

// Import basic types and functions
#include <stdlib.h>
#include <stdint.h> 

/*
Measure type:

bits:
+-----+-----------------+----------+-------------------------+
|  7  |     timing      |  6 to 0  |  Measurement Unite (SI) |
+-----+-----------------+----------+-------------------------+

where the MU can be one of the following:
	0 => meter
	1 => ampere
	2 => volt
	4 => watt
*/

#define REALTIME_MASK 0x80

typedef struct {
	uint8_t type; // type of measure: read above
	uint8_t channel; // the channel of measure, starting from 1, channel zero is reserved
	float value; // float is used because of microcontrollers limitations: https://gcc.gnu.org/wiki/avr-gcc
	uint32_t time; // the UNIX time of the measure
	uint16_t millis; // this is the offset from the given UNXI time expressed as milliseconds
} measure_t;

inline measure_t* new_nonrealtime_measure(uint8_t mu, uint8_t ch, float vl)
{
	// memory allocation
	measure_t* new_elem = (measure_t*)malloc(sizeof(measure_t));
	
	// this is a non-real-time measure
	new_elem->type = mu & (~REALTIME_MASK);
	new_elem->channel = (ch == 0) ? 1 : ch;
	new_elem->value = vl;
	
	// non real-time
	new_elem->time = 0;
	new_elem->millis = 0;
	
	// return the filled struct
    return new_elem;
}

#ifdef __HOST__
	void repack_measure(measure_t*, uint8_t*);
#else
    void unpack_measure(measure_t*, uint8_t*);
#endif

#endif // __MEASURE_H__
