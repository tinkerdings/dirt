#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>

namespace Dirt
{
  namespace Structures
  {
    struct Container
    {
      uint16_t pos[2] = {};
      uint16_t width = 0;
      uint16_t height = 0;
    };
  }
}

#endif // CONTAINER_H
