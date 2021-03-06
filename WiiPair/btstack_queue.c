/*  MFiWrapper
 *  Copyright (C) 2014 - Jason Fetters
 * 
 *  MFiWrapper is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  MFiWrapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with MFiWrapper.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "btstack.h"

struct btpad_queue_command
{
    const hci_cmd_t* command;

    union
    {
        struct
        {
            uint8_t on;
        }   btstack_set_power_mode;
   
        struct
        {
            bd_addr_t bd_addr;
            uint16_t packet_type;
            uint8_t page_scan_repetition_mode;
            uint8_t page_scan_mode;
            uint16_t clock_offset;
            uint8_t allow_role_switch;
        }   hci_create_connection;
   
        struct
        {
            uint16_t handle;
            uint8_t reason;
        }   hci_disconnect;

        struct
        {
            uint32_t lap;
            uint8_t length;
            uint8_t num_responses;
        }   hci_inquiry;

        struct
        {
            bd_addr_t bd_addr;
            uint8_t page_scan_repetition_mode;
            uint8_t reserved;
            uint16_t clock_offset;
        }   hci_remote_name_request;

        struct
        {
            uint16_t handle;
        } auth;

        struct // For wiimote only
        {
            bd_addr_t bd_addr;
            bd_addr_t pin;
        }   hci_pin_code_request_reply;
       
        struct
        {
            uint32_t a;
            uint32_t b;
        }   hci_set_event_mask;
        
        struct
        {
            uint8_t mode;
        }   hci_write_simple_pairing_mode;
    };
};

struct btpad_queue_command commands[64];
static uint32_t insert_position;
static uint32_t read_position;
static uint32_t can_run;

#define INCPOS(POS) { POS##_position = (POS##_position + 1) % 64; }

void btpad_queue_reset()
{
   insert_position = 0;
   read_position = 0;
   can_run = 1;
}

void btpad_queue_run(uint32_t count)
{
   can_run = count;

   btpad_queue_process();
}

void btpad_queue_process()
{
    for (; can_run && (insert_position != read_position); can_run --)
    {
        struct btpad_queue_command* cmd = &commands[read_position];

        if (cmd->command == &btstack_set_power_mode)
            bt_send_cmd(cmd->command, cmd->btstack_set_power_mode.on);
        else if (cmd->command == &hci_read_bd_addr)
            bt_send_cmd(cmd->command);
        else if (cmd->command == &hci_create_connection)
            bt_send_cmd(cmd->command, cmd->hci_create_connection.bd_addr,
                        cmd->hci_create_connection.packet_type,
                        cmd->hci_create_connection.page_scan_repetition_mode,
                        cmd->hci_create_connection.page_scan_mode,
                        cmd->hci_create_connection.clock_offset,
                        cmd->hci_create_connection.allow_role_switch);
        else if (cmd->command == &hci_disconnect)
            bt_send_cmd(cmd->command, cmd->hci_disconnect.handle, cmd->hci_disconnect.reason);
        else if (cmd->command == &hci_inquiry)
            bt_send_cmd(cmd->command, cmd->hci_inquiry.lap, cmd->hci_inquiry.length, cmd->hci_inquiry.num_responses);
        else if (cmd->command == &hci_remote_name_request)
            bt_send_cmd(cmd->command, cmd->hci_remote_name_request.bd_addr, cmd->hci_remote_name_request.page_scan_repetition_mode,
                        cmd->hci_remote_name_request.reserved, cmd->hci_remote_name_request.clock_offset);
        else if (cmd->command == &hci_pin_code_request_reply)
            bt_send_cmd(cmd->command, cmd->hci_pin_code_request_reply.bd_addr, 6, cmd->hci_pin_code_request_reply.pin);
        else if (cmd->command == &hci_authentication_requested)
            bt_send_cmd(cmd->command, cmd->auth.handle);
        else if (cmd->command == &hci_set_event_mask)
            bt_send_cmd(cmd->command, cmd->hci_set_event_mask.a, cmd->hci_set_event_mask.b);
        else if (cmd->command == &hci_write_simple_pairing_mode)
            bt_send_cmd(cmd->command, cmd->hci_write_simple_pairing_mode);
    
        INCPOS(read);
    }
}

void btpad_queue_btstack_set_power_mode(uint8_t on)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &btstack_set_power_mode;
   cmd->btstack_set_power_mode.on = on;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_read_bd_addr()
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_read_bd_addr;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_create_connection(bd_addr_t bd_addr, uint16_t packet_type,
                        uint8_t page_scan_repetition_mode, uint8_t page_scan_mode,
                        uint16_t clock_offset, uint8_t allow_role_switch)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_create_connection;
   memcpy(cmd->hci_create_connection.bd_addr, bd_addr, sizeof(bd_addr_t));   
   cmd->hci_create_connection.packet_type = packet_type;
   cmd->hci_create_connection.page_scan_repetition_mode = page_scan_repetition_mode;
   cmd->hci_create_connection.page_scan_mode = page_scan_mode;
   cmd->hci_create_connection.clock_offset = clock_offset;
   cmd->hci_create_connection.allow_role_switch = allow_role_switch;

   INCPOS(insert);
   btpad_queue_process();                
}

void btpad_queue_hci_disconnect(uint16_t handle, uint8_t reason)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_disconnect;
   cmd->hci_disconnect.handle = handle;
   cmd->hci_disconnect.reason = reason;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_inquiry(uint32_t lap, uint8_t length, uint8_t num_responses)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_inquiry;
   cmd->hci_inquiry.lap = lap;
   cmd->hci_inquiry.length = length;
   cmd->hci_inquiry.num_responses = num_responses;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_remote_name_request(bd_addr_t bd_addr, uint8_t page_scan_repetition_mode, uint8_t reserved, uint16_t clock_offset)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_remote_name_request;
   memcpy(cmd->hci_remote_name_request.bd_addr, bd_addr, sizeof(bd_addr_t));
   cmd->hci_remote_name_request.page_scan_repetition_mode = page_scan_repetition_mode;
   cmd->hci_remote_name_request.reserved = reserved;
   cmd->hci_remote_name_request.clock_offset = clock_offset;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_pin_code_request_reply(bd_addr_t bd_addr, bd_addr_t pin)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_pin_code_request_reply;
   memcpy(cmd->hci_pin_code_request_reply.bd_addr, bd_addr, sizeof(bd_addr_t));
   memcpy(cmd->hci_pin_code_request_reply.pin, pin, sizeof(bd_addr_t));

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_authentication_requested(uint16_t handle)
{
   struct btpad_queue_command* cmd = &commands[insert_position];

   cmd->command = &hci_authentication_requested;
   cmd->auth.handle = handle;

   INCPOS(insert);
   btpad_queue_process();
}

void btpad_queue_hci_set_event_mask(uint32_t a, uint32_t b)
{
    struct btpad_queue_command* cmd = &commands[insert_position];
    
    cmd->command = &hci_set_event_mask;
    cmd->hci_set_event_mask.a = a;
    cmd->hci_set_event_mask.b = b;
    
    INCPOS(insert);
    btpad_queue_process();
}

void btpad_queue_hci_write_simple_pairing_mode(uint8_t aMode)
{
    struct btpad_queue_command* cmd = &commands[insert_position];
    
    cmd->command = &hci_write_simple_pairing_mode;
    cmd->hci_write_simple_pairing_mode.mode = aMode;
    
    INCPOS(insert);
    btpad_queue_process();
}



