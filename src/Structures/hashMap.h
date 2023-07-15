#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <stdint.h>

namespace Dirt
{
  namespace Structures
  {
    struct HashMap
    {
      size_t objectSize = 0;
      size_t nSlots = 0;
      size_t nDupes = 0;
      uint32_t nSet = 0;
      struct Entry 
      {
        bool isSet = false;
        size_t nBytes = 0;
        uint8_t *data = 0;
      } **map = 0;
    };

    size_t hash(uint8_t *bytes, size_t len);
    size_t index(HashMap *map, uint8_t *bytes, size_t len);
    HashMap *hashMapCreate(size_t nSlots, size_t nDupes, size_t dataSize);
    bool hashMapInsertEntry(HashMap *map, void *data, size_t size);
    bool resizeHashMap(HashMap *map, size_t nObjects, size_t nDupes);
    bool isPathInHashMap(HashMap *map, char *path);
  }
}

#endif
