#include <memory/memory.h>
#include <stdint.h>

namespace Dirt
{
  namespace Memory
  {
    bool compareBytes(void *a, void *b, size_t aSize, size_t bSize)
    {
      if((aSize) != (bSize))
      {
        return false;
      }

      for(size_t i = 0; i < aSize; i++)
      {
        if(((uint8_t *)a)[i] != ((uint8_t *)b)[i])
        {
          return false;
        }
      }

      return true;
    }
  }
}
