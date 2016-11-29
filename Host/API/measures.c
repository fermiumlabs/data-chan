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

#include "../../config.h"
#include "API.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

// for repack_measure prototype
#include "../../Protocol/measure.h"

#if defined(__MACH__)
	#include <malloc/malloc.h>
#else
    #include <malloc.h>
#endif

void repack_measure(measure_t* out, uint8_t* in) {
    // the first byte is the type of measure sent
    memcpy((void*)&out->type, (const void*)in, sizeof(out->type));
    in += sizeof(out->type);

    // the second byte is the source channel
    memcpy((void*)&out->channel, (const void*)in, sizeof(out->channel));
    in += sizeof(out->channel);

    // let's continue with the value...
    memcpy((void*)&out->value, (const void*)in, sizeof(out->value));
    in += sizeof(out->value);

    // ...measurement unit...
    memcpy((void*)&out->mu, (const void*)in, sizeof(out->mu));
    in += sizeof(out->mu);

		if (out->type != NONREALTIME) {
		    // ..time...
		    memcpy((void*)&out->time, (const void*)in, sizeof(out->time));
		    in += sizeof(out->time);

		    // ..millis...
		    memcpy((void*)&out->millis, (const void*)in, sizeof(out->millis));
		    in += sizeof(out->millis);
	  } else {
			long            ms; // Milliseconds
			time_t          s;  // Seconds
			struct timespec spec;

			// get the UNIX time and millis
			clock_gettime(CLOCK_REALTIME, &spec);
			s  = spec.tv_sec;
			ms = round(spec.tv_nsec / 1.0e6);

			out->time = s;
			out->millis = ms;
	  }
}

void datachan_device_enqueue_measure(datachan_device_t* dev, const measure_t* m) {
    if ((dev == (datachan_device_t*)NULL) || (m == (const measure_t*)NULL))
        return;

    // copy the measure in a safe place
    measure_t* measure_copy = (measure_t*)malloc(sizeof(measure_t));
    memcpy((void*)measure_copy, (const void*)m, sizeof(measure_t));

    // lock on the queue and perform the insertion
    pthread_mutex_lock(&dev->measures_queue_mutex);
    enqueue_measure(&dev->measures_queue, measure_copy);
    pthread_mutex_unlock(&dev->measures_queue_mutex);
}

measure_t* datachan_device_dequeue_measure(datachan_device_t* dev) {
    if (dev == (datachan_device_t*)NULL)
        return (measure_t*)NULL;

    // the measure
    measure_t* measure = (measure_t*)NULL;

    // lock on the queue and perform the deletion
    pthread_mutex_lock(&dev->measures_queue_mutex);
    measure = dequeue_measure(&dev->measures_queue);
    pthread_mutex_unlock(&dev->measures_queue_mutex);

    return measure;
}

int32_t datachan_device_enqueued_measures(datachan_device_t* dev) {
    if (dev == (datachan_device_t*)NULL)
        return -1;

    uint32_t count = 0;

    // lock on the queue and get the actual measures count
    pthread_mutex_lock(&dev->measures_queue_mutex);
    count = count_measures(&dev->measures_queue);
    pthread_mutex_unlock(&dev->measures_queue_mutex);

    return (int32_t)count;
}
