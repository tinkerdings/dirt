#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <structures/hashmap.h>
#include <error/errorCode.h>
#include <memory/memory.h>

namespace Dirt
{
  namespace Structures
  {
    // djb2 hash function
    size_t hash(uint8_t *bytes, size_t len)
    {
      uint32_t hash = 5381;
      for(uint32_t i = 0; i < len; i++)
      {
        hash = ((hash << 5) + hash) + bytes[i];
      }

      return hash;
    }

    size_t index(Hashmap *map, uint8_t *bytes, size_t len)
    {
      return hash(bytes, len) % map->nSlots;
    }

    Hashmap *hashmapCreate(size_t nSlots, size_t nDupes, size_t dataSize)
    {
      Hashmap *map = 0;
      if(!(map = (Hashmap *)malloc(sizeof(Hashmap))))
      {
        return 0;
      }

      if(!(map->map = (Hashmap::Entry **)calloc(nSlots, sizeof(Hashmap::Entry *))))
      {
        free(map);
        return 0;
      }

      for(size_t i = 0; i < nSlots; i++)
      {
        if(!(map->map[i] = (Hashmap::Entry *)calloc(nDupes, sizeof(Hashmap::Entry))))
        {
          for(size_t e = i; i >= 0; e--)
          {
            free(map->map[e]);
          }
          free(map->map);
          free(map);
          return 0;
        }
        for(size_t j = 0; j < nDupes; j++)
        {
          if(!(map->map[i][j].data = (uint8_t *)calloc(dataSize, sizeof(uint8_t))))
          {
            for(size_t e = j; e >= 0; e--)
            {
              free(map->map[i][j].data);
            }
            for(size_t e = 0; e < nSlots; e++)
            {
              free(map->map[e]);
            }
            free(map->map);
            free(map);
            return 0;
          }
          map->map[i][j].nBytes = dataSize;
          map->map[i][j].isSet = false;
        }
      }

      map->nSet = 0;
      map->nSlots = nSlots;
      map->nDupes = nDupes;
      map->dataSize = dataSize;

      return map;
    }

    bool hashmapInsertEntry(Hashmap *map, void *data, size_t size)
    {
      size_t hashIndex = 
        index(map, (uint8_t *)data, size);

      if(map->nSet >= map->nSlots)
      {
        hashmapResize(map, map->nSlots*2, map->nDupes);
      }

      bool foundFreeSpot = false;
      int i;
      for(i = 0; i < map->nDupes; i++)
      {
        if((foundFreeSpot = (!map->map[hashIndex][i].isSet)))
        {
          break;
        }
      }

      if(!foundFreeSpot)
      {
        hashmapResize(map, map->nSlots, map->nDupes * 2);
        memset((uint8_t *)map->map[hashIndex][i+1].data, 0, size);
        memcpy((uint8_t *)map->map[hashIndex][i+1].data, data, size);
        map->map[hashIndex][i+1].isSet = true;
      }
      else 
      {
        memset((uint8_t *)map->map[hashIndex][i].data, 0, size);
        memcpy((uint8_t *)map->map[hashIndex][i].data, data, size);
        map->map[hashIndex][i].isSet = true;
        if(i == 0)
        {
          map->nSet++;
        }
      }

      return true;
    }

    bool hashmapResize(Hashmap *map, size_t nSlots, size_t nDupes)
    {
      size_t nSlotsOld = map->nSlots;

      if(nSlots != map->nSlots)
      {
        Hashmap::Entry **tmpPtr = (Hashmap::Entry **)realloc(map->map, nSlots * sizeof(Hashmap::Entry *));
        if(!tmpPtr)
        {
          Dirt::Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }
        map->map = tmpPtr;
        map->nSlots = nSlots;
      }

      for(size_t i = nSlotsOld; i < nSlots; i++)
      {
        if(!(map->map[i] = (Hashmap::Entry *)malloc(nDupes * sizeof(Hashmap::Entry))))
        {
          Dirt::Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }

        for(size_t j = 0; j < nDupes; j++)
        {
          if(!(map->map[i][j].data = (uint8_t *)calloc(map->dataSize, sizeof(uint8_t))))
          {
            return false;
          }
        }
      }

      if(nDupes != map->nDupes)
      {
        for(size_t i = 0; i < nSlotsOld; i++)
        {
          Hashmap::Entry *tmpPtr = (Hashmap::Entry *)realloc(map->map[i], nDupes * sizeof(Hashmap::Entry));
          if(!tmpPtr)
          {
            Dirt::Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          }
          map->map[i] = tmpPtr;
          for(size_t j = map->nDupes; j < nDupes; j++)
          {
            if(!(map->map[i][j].data = (uint8_t *)calloc(map->dataSize, sizeof(uint8_t))))
            {
              return false;
            }
          }
        }
        map->nDupes = nDupes;
      }

      return true;
    }

    bool hashmapRemove(Hashmap *map, void *data, size_t dataSize)
    {
      size_t hashIndex = 0;
      size_t dupeIndex = 0;
      if(!hashmapContains(map, data, dataSize, hashIndex, dupeIndex))
      {
        return false;
      }

      size_t freeSpot;
      for(freeSpot = 0; freeSpot < map->nDupes; freeSpot++)
      {
        if(!map->map[hashIndex][freeSpot].isSet)
        {
          break;
        }
      }

      if(freeSpot == 1)
      {
        memset(map->map[hashIndex][dupeIndex].data, 0, map->map[hashIndex][dupeIndex].nBytes);
        map->map[hashIndex][dupeIndex].isSet = false;
        map->map[hashIndex][dupeIndex].nBytes = 0;
        return true;
      }

      size_t j;
      for(j = dupeIndex+1; j < freeSpot; j++)
      {
        memcpy(map->map[hashIndex][j-1].data, map->map[hashIndex][j].data, map->map[hashIndex][j].nBytes);
      }
      memset(map->map[hashIndex][j].data, 0, map->map[hashIndex][j].nBytes);
      map->map[hashIndex][j].isSet = false;
      map->map[hashIndex][j].nBytes = 0;
      return true;
    }

    bool hashmapContains(Hashmap *map, void *data, size_t dataSize, size_t &hashIndex, size_t &dupeIndex)
    {
      size_t idx = index(map, (uint8_t *)data, dataSize);
      if(map->map[idx][0].isSet)
      {
        size_t i;
        for(i = 0; i < map->nDupes; i++)
        {
          if(Dirt::Memory::compareBytes(map->map[idx][i].data, data, map->map[idx][i].nBytes, dataSize))
          {
            hashIndex = idx;
            dupeIndex = i;
            return true;
          }
        }
      }

      return false;
    }

    /* // Extra */
    /* bool hashmapAddPathString(Hashmap *map, char *path) */
    /* { */
    /*   char entryFullPath[MAX_PATH] = {0}; */
    /*   if(!getFullPath(entryFullPath, path, MAX_PATH)) */
    /*   { */
    /*     Dirt::Error::errorCode = DIRT_ERROR_GETFULLPATH; */
    /*     return false; */
    /*   } */

    /*   DWORD entryAttributes = GetFileAttributesA(entryFullPath); */
    /*   if(entryAttributes == INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_SHARING_VIOLATION) */
    /*   { */
    /*     Dirt::Error::errorCode = DIRT_ERROR_INVALID_ENTRY; */
    /*     return false; */
    /*   } */

    /*   hashmapAdd(map, entryFullPath, strlen(entryFullPath)); */
    /* } */
  }
}
