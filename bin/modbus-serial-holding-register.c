//------------------------------------------------------------------------------
// Copyright (C) 2014, Robert Johansson <rob@raditex.nu>, Raditex Control AB
// All rights reserved.
//
// This file is part of rSCADA.
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>

#include <modbus/modbus.h>
#include <modbus/modbus-serial.h>

static int debug = 1;

void
usage(char *program_name)
{
    printf("usage: %s DEVICE COMMAND UNIT ADDRESS [RANGE] [VALUE]\n", program_name);
    printf("\tDEVICE   = a TTY device\n");
    printf("\tCOMMAND = read | write\n");    
    printf("\tUNIT    = address of unit\n");    
    printf("\tADDRESS = 0x0000 - 0xFFFF (register address)\n");
    printf("\tRANGE   = 0x0000 - 0xFFFF (for command = read)\n");    
    printf("\tVALUE   = 0x0000 - 0xFFFF (for command = write)\n");    
    exit(0);
}

//------------------------------------------------------------------------------
// Read/write a holding register form a MODBUS SERIAL device
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    static char *device, *command;
    long baudrate = 57600;   // Select for this device
    uint16_t unit, addr, value, range, i, verbose = 1;
    
    modbus_frame_t     *pkt;    
    modbus_serial_handle_t *handle;
 
    if (argc != 5 && argc != 6)
    {
        printf("Error: Invalid arguments.\n\n");    
        usage(argv[0]);
    }
 
    device  = argv[1];
    command = argv[2];
    unit = atoi(argv[3]);
    
    // setup request frame
    pkt = modbus_frame_new();      
    
    if (strcmp(command, "read") == 0)
    {
        addr = atoi(argv[4]);
        range = (argc == 6) ? atoi(argv[5]) : 1;
        
        modbus_read_holding_registers(pkt, addr, range);          
    }
    else if (strcmp(command, "write") == 0 && argc == 6)
    {
        addr  = atoi(argv[4]);
        value = atoi(argv[5]);        
        
        modbus_preset_single_register(pkt, addr, value);  
    }    
    else
    {
        printf("Error: Invalid COMMAND: %s\n\n", command);
        usage(argv[0]);
    }
    
    modbus_frame_set_unit(pkt, unit);

    // setup serial connection
    if ((handle = modbus_serial_connect(device, baudrate)) == NULL)
    {
        printf("%s: modbus_serial_connect failed: %s.\n", __PRETTY_FUNCTION__, modbus_error_str);
        return 0;
    }

    if (debug)
        modbus_rtu_frame_print(pkt);
        
    // Send command
    if (modbus_serial_send(handle, pkt) != 0)
    {
        printf("%s: modbus_serial_send failed: %s.\n", __PRETTY_FUNCTION__, modbus_error_str);
        return 0;       
    }

    // Question:
    // does modbus_serial_recv wait for the packet?
    // Is there any select on the file descriptor?? 

    // recv response
    if (modbus_serial_recv(handle, pkt) != 0)
    {
        printf("%s: modbus_serial_recv failed: %s.\n", __PRETTY_FUNCTION__, modbus_error_str);
        return 0;       
    }

    if (modbus_frame_verify(pkt) == -1)
    {
        printf("%s: modbus_frame_verify failed: %s\n", __PRETTY_FUNCTION__, modbus_error_str);
        return 0;               
    }

    if (debug)
        modbus_rtu_frame_print(pkt);

    if (strcmp(command, "read") == 0)
    {
        for (i = 0; i < range; i++)
        {
            value = (pkt->data_buff[3] * 255 + pkt->data_buff[4]);
            value = modbus_frame_data_decode16(pkt, 1 + 2*i);
            
            if (verbose)
            {
                printf("Holding register value [address = 0x%.4x] = 0x%.4x\n", addr + i, value);
            }
            else
            {
                printf("%.4x ", addr + i, value);
            }
        }
    }

    modbus_frame_free(pkt);

    return 0;
}



