#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>

namespace Dirt
{
  namespace Structures
  {
    struct Hashmap
    {
      size_t dataSize = 0;
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
    size_t index(Hashmap *map, uint8_t *bytes, size_t len);
    Hashmap *hashmapCreate(size_t nSlots, size_t nDupes, size_t dataSize);
    bool hashmapInsertEntry(Hashmap *map, void *data, size_t dataSize);
    bool hashmapRemove(Hashmap *map, void *data, size_t dataSize);
    bool hashmapResize(Hashmap *map, size_t nObjects, size_t nDupes);
    bool hashmapContains(Hashmap *map, void *data, size_t dataSize, size_t &hashIndex, size_t dupeIndex);
  }
}

#endif // HASHMAP_H
