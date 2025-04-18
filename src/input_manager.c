#include "../include/input_manager.h"

#include <IOKit/hid/IOHIDLib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/debug.h"

// macOS implementation of input manager
static IOHIDManagerRef inputManager = NULL;

int init_input_manager(void)
{
    inputManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!inputManager)
    {
        debugf(stderr, "Failed to create IOHIDManager\n");
        return -1;
    }

    IOHIDManagerOpen(inputManager, kIOHIDOptionsTypeNone);
    return 0;
}

void cleanup_input_manager(void)
{
    if (inputManager)
    {
        IOHIDManagerClose(inputManager, kIOHIDOptionsTypeNone);
        CFRelease(inputManager);
        inputManager = NULL;
    }
}