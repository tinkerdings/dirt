#ifndef ERRORCODE_H
#define ERRORCODE_H

#include <stdint.h>

#define DIRT_ERROR_GETFULLPATH 0xAA
#define DIRT_ERROR_ALLOCATION_FAILURE 0x2
#define DIRT_ERROR_INVALID_ENTRY 0x4
#define DIRT_ERROR_SPLITBOX_ALREADY_SPLIT 0x0100

namespace Dirt
{
  namespace Error
  {
    extern uint32_t errorCode;
  }
}

#endif ERRORCODE_H
