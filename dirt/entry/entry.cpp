#include <dirt/context/context.h>
#include <dirt/entry/entry.h>
#include <dirt/structures/hashmap.h>
#include <dirt/error/errorCode.h>
#include <shlwapi.h>
#include <windows.h>
#include <stdio.h>

namespace Dirt
{
  namespace Structures
  {
    struct Hashmap;
  }

  namespace Entry
  {
    bool getFullPath(char *out, char* relPath, size_t outLen)
    {
      char fullPath[MAX_PATH] = {0};
      GetFullPathNameA(relPath, MAX_PATH, fullPath, 0);
      if(strlen(fullPath) > outLen)
      {
        out = 0;
        return false;
      }
      strcpy(out, fullPath);

      return true;
    }

    // Searches specified directory for files and directories,
    // returns number of entries found, and stores entries in <entries> argument
    WIN32_FIND_DATA *findDirectoryEntries(Context *context, char *dirPath, size_t &nEntries)
    {
      if((strlen(dirPath)+3) > MAX_PATH)
      {
        printf("path length exceeds limit\n");
        return 0;
      }
      HANDLE entry = INVALID_HANDLE_VALUE;
      WIN32_FIND_DATA *entries = (WIN32_FIND_DATA *)malloc(context->entryBufferNSlots * sizeof(WIN32_FIND_DATA));
      char fullPath[MAX_PATH] = {0};
      GetFullPathName(dirPath, MAX_PATH, fullPath, 0);
      strcat(fullPath, "\\*");

      entry = FindFirstFile(fullPath, &entries[0]);
      if(entry == INVALID_HANDLE_VALUE)
      {
        DWORD fileAttribs = GetFileAttributesA(fullPath);
        if(!fileAttribs)
        {
          printf("GetFileAttributesA failed (%lu)\n", GetLastError());
          return 0;
        }
        if(fileAttribs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
          // TODO: Handle this
        }
        else 
        {
          printf("FindFirstFile failed (%lu)\n", GetLastError());
          free(entries);
          return 0;
        }
      }

      size_t i = 1;
      BOOL findSuccess = 1;

      while(1)
      {
        while((i < context->entryBufferNSlots) &&
            (findSuccess = FindNextFile(entry, &entries[i++]))){}
        DWORD error;

        if(!findSuccess && ((error = GetLastError()) != ERROR_NO_MORE_FILES))
        {
          printf("FindNextFile failed (%lu)\n", error);
          free(entries);
          return 0;
        }
        else 
        {
          if(error == ERROR_NO_MORE_FILES)
          {
            break;
          }
          context->entryBufferNSlots *= 2;
          WIN32_FIND_DATA *tmpPtr = (WIN32_FIND_DATA *)realloc(entries, context->entryBufferNSlots * sizeof(WIN32_FIND_DATA));
          if(!tmpPtr)
          {
            printf("Failed to realloc entries in findDirectoryEntries\n");
            return 0;
          }
          else 
          {
            entries = tmpPtr;
          }
        }
      }

      FindClose(entry);

      nEntries = --i;

      return entries;
    }

    bool removeEntryFromSelection(Context *context, char *path)
    {
      if(!Structures::hashmapRemove(context->selection, path, MAX_PATH))
      {
        printf("hashmapRemove failed\n");
        return false;
      }

      return true;
    }

    char **getSelection(Context *context, int &amountOut)
    {
      int bufSize = context->selection->nSlots;
      char **selected = 0;
      if(!(selected = (char **)calloc(bufSize, sizeof(char *))))
      {
        printf("calloc failed to allocate array of strings for selected entries\n");
        return 0;
      }
      for(int i = 0; i < context->selection->nSlots; i++)
      {
        if(!(selected[i] = (char *)calloc(MAX_PATH, sizeof(char))))
        {
          printf("calloc failed to allocate string for selected entry array selected[i]\n");
          for(int j = i-1; j >= 0; j--)
          {
            free(selected[j]); // TODO do this kind of freeing in other places.
          }
          free(selected);
          return 0;
        }
      }

      int idx = 0;

      for(int i = 0; i < context->selection->nSlots; i++)
      {
        if(context->selection->map[i][0].isSet)
        {
          for(int j = 0; j < context->selection->nDupes; j++)
          {
            if(!context->selection->map[i][j].isSet)
            {
              break;
            }
            if(idx >= bufSize)
            {
              bufSize *= 2;
              char **tmpPtr = (char **)realloc(selected, bufSize);
              if(!tmpPtr)
              {
                printf("realloc failed to allocate array of strings for selected entries\n");
                Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
                return 0;
              }
              else 
              {
                for(int k = idx; k < context->selection->nSlots; k++)
                {
                  if(!(selected[k] = (char *)calloc(MAX_PATH, sizeof(char))))
                  {
                    printf("realloc, then calloc failed to allocate string for selected entry array selected[i]\n");
                    for(int h = k-1; h >= 0; h--)
                    {
                      free(selected[h]); // TODO do this kind of freeing in other places.
                    }
                    free(selected);
                    return 0;
                  }
                }
              }
            }

            memcpy(selected[idx], context->selection->map[i][j].data, context->selection->dataSize);
            idx++;
          }
        }
      }

      amountOut = idx;
      return selected;
    }

    void freeSelection(char **selection, int amount)
    {
      for(int i = 0; i < amount; i++)
      {
        free(selection[i]);
      }

      free(selection);
      selection = 0;
    }

    void printSelection(Context *context)
    {
      int nSelected = 0;
      char **selection = getSelection(context, nSelected);
      if(!selection)
      {
        printf("No selection\n");
        return;
      }

      printf("\nSelected:\n");
      char prefix[3] = {0};
      for(int i = 0; i < nSelected; i++)
      {
        printf("%s%s", prefix, selection[i]);
        prefix[0] = ',';
        prefix[1] = '\n';
      }
      printf("\n");

      freeSelection(selection, nSelected);
    }

    bool moveSelection(Context *context)
    {
      int nSelected = 0;
      char **selection = getSelection(context, nSelected);

      char currentDir[MAX_PATH] = {0};
      if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
      {
        printf("GetCurrentDirectory failed (%lu)\n", GetLastError());
        return false;
      }

      for(int i = 0; i < nSelected; i++)
      {
        DWORD fileAttribs = GetFileAttributesA(selection[i]);
        if(!fileAttribs)
        {
          printf("GetFileAttributesA failed (%lu)\n", GetLastError());
          continue;
        }

        char *filename = PathFindFileNameA(selection[i]);
        char destination[MAX_PATH] = {0};
        strcpy(destination, currentDir);
        strcat(destination, "\\");
        strcat(destination, filename);

        if(!MoveFileA(selection[i], destination))
        {
          printf("MoveFileA failed (%lu)\n", GetLastError());
          return false;
        }
      }

      freeSelection(selection, nSelected);

      return true;
    }

    bool deleteSelection(Context *context)
    {
      int nSelected = 0;
      char **selection = getSelection(context, nSelected);

      char currentDir[MAX_PATH] = {0};
      if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
      {
        printf("GetCurrentDirectory failed (%lu)\n", GetLastError());
        return false;
      }

      for(int i = 0; i < nSelected; i++)
      {
        DWORD fileAttribs = GetFileAttributesA(selection[i]);
        if(!fileAttribs)
        {
          printf("GetFileAttributesA failed (%lu)\n", GetLastError());
          continue;
        }

        if(fileAttribs & FILE_ATTRIBUTE_DIRECTORY)
        {
          SHFILEOPSTRUCT fileOperation =
          {
            0,
            FO_DELETE,
            selection[i],
            0,
            FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
            false,
            0,
            0
          };
          int ret;
          if((ret = SHFileOperation(&fileOperation)))
          {
            printf("SHFileOperation failed (%lu)\n", ret);
            return false;
          }
        }
        else
        {
          if(!DeleteFileA(selection[i]))
          {
            if(fileAttribs & FILE_ATTRIBUTE_READONLY)
            {
              if(!SetFileAttributesA(selection[i], FILE_ATTRIBUTE_NORMAL))
              {
                printf("SetFileAttributesA failed (%lu)\n", GetLastError());
                return false;
              }
              if(!DeleteFileA(selection[i]))
              {
                printf("A DeleteFileA failed (%lu)\n", GetLastError());
                return false;
              }
            }
            printf("B DeleteFileA failed (%lu)\n", GetLastError());
            return false;
          }
        }
      }

      freeSelection(selection, nSelected);

      return true;
    }

    bool clearAllSelection(Context *context)
    {
      int nSelected = 0;
      char **selection = getSelection(context, nSelected);

      if(!selection)
      {
        printf("Failed to retrieve selection\n");
        return false;
      }

      for(int i = 0; i < nSelected; i++)
      {
        if(!removeEntryFromSelection(context, selection[i]))
        {
          return false;
        }
      }

      return true;
    }
  } // namespace Entry
} // namespace Dirt

