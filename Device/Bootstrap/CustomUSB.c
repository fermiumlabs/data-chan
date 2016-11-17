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

#include "../../Protocol/measure_functions.h"
#include "../../Protocol/data_management_functions.h"
#include "CustomUSB.h"
#include "Settings.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static bool hostListening = false;

void unpack_measure(measure_t* in, uint8_t* out) {
    // the first byte is the type of measure sent
    memcpy(out, (const void*)&in->type, sizeof(in->type));
    out += sizeof(in->type);

    // the second byte is the source channel
    memcpy((out), (const void*)&in->channel, sizeof(in->channel));
    out += sizeof(in->channel);

    // let's continue with the value...
    memcpy((out), (const void*)&in->value, sizeof(in->value));
    out += sizeof(in->value);

    // ...measurement unit...
    memcpy(out, (const void*)&in->mu, sizeof(in->mu));
    out += sizeof(in->mu);

    // ..time...
    memcpy((out), (const void*)&in->time, sizeof(in->time));
    out += sizeof(in->time);

    // ..millis...
    memcpy((out), (const void*)&in->millis, sizeof(in->millis));
    out += sizeof(in->millis);
}

static struct request_t *cmd_queue;

static managed_queue_t FIFO;

void datachan_init(void) {
    cmd_queue = (struct request_t*)NULL;
    FIFO.first = (struct fifo_queue_t *)NULL;
    FIFO.last = (struct fifo_queue_t *)NULL;
}

/** Function to create the next report to send back to the host at the next reporting interval.
 *
 *  \param[out] DataArray  Pointer to a buffer where the next report data should be stored
 */
void CreateGenericHIDReport(uint8_t* DataArray)
{
    /*
    This is where you need to create reports to be sent to the host from the device. This
    function is called each time the host is ready to accept a new report. DataArray is
    an array to hold the report to the host.
    */

    // every unused byte will be 00
    memset((void*)DataArray, 0x00, GENERIC_REPORT_SIZE);

    // by default nothing is sent
    DataArray[0] = (uint8_t)NONE;	

    if (hostListening) {
        // testing purpouse ONLY!
        enqueue_measure(&FIFO, new_nonrealtime_measure(0xFF, 1, 169.754699f));

        // async requests manager
        /*if (cmd_queue != (struct request_t*)NULL) {
            // point to the first byte after the response type
            uint8_t *response_builder = DataArray + 1;
            
            // get the async request to be fulfilled and remove it from the queue
            struct request_t* req = cmd_queue;
            req->next = (struct request_t*)NULL;
            cmd_queue = cmd_queue->next;

            // flag the response as an async request response
            DataArray[0] = CMD_ASYNC_RESPONSE;            

            // write the ID of the request
            memcpy((void*)response_builder, (const void*)&req->id, sizeof(req->id));
            response_builder += sizeof(req->id);

            // generate the buffer response
            uint8_t buffer[GENERIC_REPORT_SIZE - 1 - sizeof(req->id)];

            // call the external function that will generate the response
            

            // apply the generated response
            memcpy((void*)response_builder, buffer, sizeof(buffer));            

            // remove from the memory the fulfilled async command
            if (req->buffer != NULL) free((void*)req->buffer);
            free((void*)req);
        } else */
        // get the next measure to be sent over USB
        measure_t* data_to_be_sent = dequeue_measure(&FIFO);
        
        if (data_to_be_sent != (measure_t*)NULL) {
            // if any flag as present, else flag as 'bad data'
            DataArray[0] = (uint8_t)MEASURE;

            // serialize the measure (for safe transmission)
            unpack_measure(data_to_be_sent, (DataArray + 1));
            
            // the measure is going to be removed from memory
            free((void*)data_to_be_sent); // save space!
        } else {
            DataArray[0] = ((uint8_t)NONE);
        }
    }
    
    // append the error check byte to the end of EVERY message
    DataArray[GENERIC_REPORT_SIZE - 1] = CRC_calc(DataArray, GENERIC_REPORT_SIZE - 1);
}

/** Function to process the last received report from the host.
 *
 *  \param[in] DataArray  Pointer to a buffer where the last received report has been stored
 */
void ProcessGenericHIDReport(uint8_t* DataArray)
{
    /*
        This is where you need to process reports sent from the host to the device. This
        function is called each time the host has sent a new report. DataArray is an array
        holding the report sent from the host.
    */
    
    if ((DataArray[0] == CMD_REQUEST) && (CRC_check(DataArray, GENERIC_REPORT_SIZE - 1, DataArray[GENERIC_REPORT_SIZE - 1]))) {
        uint8_t cmd = DataArray[1];
        
        uint32_t entry;
        uint16_t data_size = 1;
        uint8_t channel;
        void* data;
        uint8_t *cmd_builder_buffer = DataArray + 2;
        
        switch (cmd) {
            case ENABLE_TRANSMISSION:
                hostListening = true;
                break;
                
            case DISABLE_TRANSMISSION:
                hostListening = false;
                break;

            case SET_CONFIG_FLAG:
                // read the setting ID
                memcpy((void*)&entry, (const void*)cmd_builder_buffer, sizeof(entry));
                cmd_builder_buffer += sizeof(entry);
                
                // read the destination channel
                memcpy((void*)&channel, (const void*)cmd_builder_buffer, sizeof(channel));
                cmd_builder_buffer += sizeof(channel);
                
                // read the data length
                memcpy((void*)data_size, (const void*)cmd_builder_buffer, sizeof(data_size));
                cmd_builder_buffer += sizeof(data_size);
                
                // if the length is correct allocate memory
                if (data_size < (GENERIC_REPORT_SIZE - 10)) data = malloc(data_size);
                
                // read the data
                memcpy((void*)data, (const void*)cmd_builder_buffer, data_size);
                
                // build the setting data
                memory_block_t mem;
                mem.size = data_size;
                mem.start_addr = malloc(mem.size);
                memcpy(mem.start_addr, (const void*)&data, mem.size);
                setting_entry_t new_set;
                new_set.channel = channel;
                new_set.entry = entry;
                memcpy((void*)(&new_set.setting), (const void*)&mem, sizeof(mem));
                setSetting(&new_set);
                break;

            case CMD_ASYNC_REQUEST:
                // get and store the ID of the request
                memcpy((void*)&entry, (void*)cmd_builder_buffer, sizeof(uint32_t));
                cmd_builder_buffer += sizeof(uint32_t);
                
                // get and store the request
                data = malloc(GENERIC_REPORT_SIZE - sizeof(uint32_t) - 2);
                memcpy(data, (void*)cmd_builder_buffer, GENERIC_REPORT_SIZE - sizeof(uint32_t) - 2);
                
                // create and populate the structure that will hold the request
                struct request_t *new_request = (struct request_t*)malloc(sizeof(struct request_t));
                new_request->id = entry;
                new_request->buffer = data;
                new_request->next = (struct request_t*)NULL;                

                // append the previous list to the newly created element
                struct request_t* last_populated = cmd_queue;
                new_request->next = last_populated;
                
                // exchange the old list with the new one
                cmd_queue = new_request;
                break;
                
            default:
                break;
        }
    }
}

