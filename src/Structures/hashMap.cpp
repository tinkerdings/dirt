#include <string.h>
#include <stdlib.h>

#include "hashMap.h"

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

    size_t index(HashMap *map, uint8_t *bytes, size_t len)
    {
      return hash(bytes, len) % map->nSlots;
    }

    HashMap *hashMapCreate(size_t nSlots, size_t nDupes, size_t dataSize)
    {
      HashMap *map = 0;
      if(!(map = (HashMap *)malloc(sizeof(HashMap))))
      {
        return 0;
      }

      if(!(map->map = (HashMap::Entry **)calloc(nSlots, sizeof(HashMap::Entry *))))
      {
        return 0;
      }

      for(int i = 0; i < nSlots; i++)
      {
        if(!(map->map[i] = (HashMap::Entry *)calloc(nDupes, sizeof(HashMap::Entry))))
        {
          return 0;
        }
        for(int j = 0; j < nDupes; j++)
        {
          if(!(map->map[i][j].data = (uint8_t *)calloc(dataSize, sizeof(uint8_t))))
          {
            return 0;
          }
          map->map[i][j].nBytes = dataSize;
        }
      }

      map->nSet = 0;
      map->nSlots = nSlots;
      map->nDupes = nDupes;

      return map;
    }

    bool hashMapInsertEntry(HashMap *map, void *data, size_t size)
    {
      size_t hashIndex = 
        hash((uint8_t *)data, size) % map->nSlots;

      if(map->nSet >= map->nSlots)
      {
        resizeHashMap(map, map->nSlots*2, map->nDupes);
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
        resizeHashMap(map, map->nSlots, map->nDupes * 2);
        memcpy((uint8_t *)map->map[hashIndex][i+1].data, data, size);
        map->map[hashIndex][i+1].isSet = true;
      }
      else 
      {
        memcpy((uint8_t *)map->map[hashIndex][i].data, data, size);
        map->map[hashIndex][i].isSet = true;
        if(i == 0)
        {
          map->nSet++;
        }
      }

      return true;
    }


    bool resizeHashMap(HashMap *map, size_t nObjects, size_t nDupes)
    {
      if(nObjects != map->nObjects)
      {
        HashMap::Entry **tmpPtr = (HashMap::Entry **)realloc(map->map, nObjects);
        if(!tmpPtr)
        {
          printf("realloc failed on resizeHashMap for map->map\n");
          errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }
        map->map = tmpPtr;
        map->nObjects = nObjects;
      }

      size_t nObjectsOld = map->nObjects;

      for(size_t i = nObjectsOld; i < nObjects; i++)
      {
        if(!(map->map[i] = (HashMap::Entry *)malloc(nDupes * sizeof(HashMap::Entry))))
        {
          errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }

        for(size_t j = 0; j < nDupes; j++)
        {
          if(!(map->map[i][j].data = (uint8_t *)calloc(map->objectSize, sizeof(uint8_t))))
          {
            printf("calloc failed on resizeHashMap for map->map[nObjectsOld+i][j]\n");
            return false;
          }
        }
      }

      if(nDupes != map->nDupes)
      {
        for(size_t i = 0; i < nObjectsOld; i++)
        {
          HashMap::Entry *tmpPtr = (HashMap::Entry *)realloc(map->map[i], nDupes * sizeof(HashMap::Entry));
          if(!tmpPtr)
          {
            printf("realloc failed on resizeHashMap for map->map[0+i]\n");
            errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          }
          map->map[i] = tmpPtr;
          for(size_t j = map->nDupes; j < nDupes; j++)
          {
            if(!(map->map[i][j].data = (uint8_t *)calloc(map->objectSize, sizeof(uint8_t))))
            {
              printf("calloc failed on resizeHashMap for map->map[0+i][j]\n");
              return false;
            }
          }
        }
      }

      map->nDupes = nDupes;
      return true;
    }

    bool hashMapRemove(HashMap *map, void *entry)
    {
      if(!isPathInHashMap(map, entry))
      {
        printf("Entry not in selection\n");
        return false;
      }

      int hashIndex = hash(entry, strlen(entry)) % map->nObjects;

      for(int i = 0; i < map->nDupes; i++)
      {
        if(!strcmp(entry, (char *)map->map[hashIndex][i]))
        {
          int lastDupe = 0;
          for(int j = i+1; j < map->nDupes; j++)
          {
            if(!map->map[hashIndex][j][0])
            {
              lastDupe = j-1;
              break;
            }
          }
          if(lastDupe != i)
          {
            int j = i;
            int k = i+1;
            for(; k < lastDupe-1; j++, k++)
            {
              strcpy((char *)map->map[hashIndex][j], (char *)map->map[hashIndex][k]);
            }
            map->map[hashIndex][k][0] = '\0';
          }
          else 
          {
            map->map[hashIndex][i][0] = '\0';

            if(i == 0)
            {
              map->nSet--;
            }
          }

          return true;
        }
      }

      printf("Something went horribly wrong\n");
      return false;
    }

    bool HashMapContains(HashMap *map, void *data, size_t size)
    {
      /* char fullPath[MAX_PATH] = {0}; */
      /* if(!getFullPath(fullPath, path, MAX_PATH)) */
      /* { */
      /*   printf("getFullPath failed\n"); */
      /*   errorCode = DIRT_ERROR_GETFULLPATH; */
      /*   return false; */
      /* } */

      size_t hashId = hash((uint8_t *)data, size) % map->nObjects;
      if(map->map[hashId][0].isSet)
      {
        for(int i = 0; i < map->nDupes; i++)
        {
          if(!strcmp((char *)map->map[hashId][i].data, fullPath))
          {
            return true;
          }
        }
      }

      return false;
    }

    // Extra
    bool compareBytes(void *a, void *b, size_t aSize, size_t bSize)
    {

    }

    // Extra
    bool hashMapAddPathString(HashMap *map, char *path)
    {
      char entryFullPath[MAX_PATH] = {0};
      if(!getFullPath(entryFullPath, path, MAX_PATH))
      {
        printf("getFullPath failed in hashMapAdd\n");
        errorCode = DIRT_ERROR_GETFULLPATH;
        return false;
      }

      DWORD entryAttributes = GetFileAttributesA(entryFullPath);
      if(entryAttributes == INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_SHARING_VIOLATION)
      {
        printf("GetFileAttributesA failed (%lu)\n", GetLastError());
        errorCode = DIRT_ERROR_INVALID_ENTRY;
        return false;
      }

      hashMapAdd(map, entryFullPath, strlen(entryFullPath));
    }
  }
}
