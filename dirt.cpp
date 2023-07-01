#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <winerror.h>

WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries);

int main(int argc, char **argv)
{

  return 0;
}

// Searches specified directory for files and directories,
// returns number of entries found, and stores entries in <entries> argument
WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries)
{
  HANDLE entry = INVALID_HANDLE_VALUE;
  size_t maxEntries = 128;
  WIN32_FIND_DATA *entries = (WIN32_FIND_DATA *)malloc(maxEntries * sizeof(WIN32_FIND_DATA));

  entry = FindFirstFile(dirPath, &entries[0]);
  if(entry == INVALID_HANDLE_VALUE)
  {
    printf("FindFirstFile failed (%lu\n", GetLastError());
    free(entries);
    return 0;
  }

  size_t i = 1;
  BOOL findSuccess = 1;
  while(1)
  {
    while((i < maxEntries) && (findSuccess = FindNextFile(entry, &entries[i++]))){}
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
      maxEntries *= 2;
      realloc(entries, maxEntries);
    }
  }

  FindClose(entry);

  nEntries = i--;

  return entries;
}
