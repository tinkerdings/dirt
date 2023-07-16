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
        uint8_t *data = 0;
      } **map = 0;
    };

    uint32_t hash(void *bytes, uint32_t len);
    uint32_t hashmapGetIndex(Hashmap *map, void *data, size_t dataSize);
    Hashmap *hashmapCreate(size_t nSlots, size_t nDupes, size_t dataSize);
    void hashmapDestroy(Hashmap *map);
    bool hashmapInsert(Hashmap *map, void *data, size_t dataSize);
    bool hashmapInsertAtKey(Hashmap *map, void *key, void *data, size_t keySize, size_t dataSize);
    bool hashmapRemove(Hashmap *map, void *data, size_t dataSize);
    bool hashmapResize(Hashmap *map, size_t nObjects, size_t nDupes);
    bool hashmapContains(Hashmap *map, void *data, size_t dataSize, size_t *hashIndex, size_t *dupeIndex);
    size_t hashmapGetFreeDupeIndex(Hashmap *map, size_t hashIndex);
    bool hashmapDirectWrite(Hashmap *map, void *data, size_t hashIndex, size_t dupeIndex, size_t dataSize);
  }
}

#endif // HASHMAP_H
