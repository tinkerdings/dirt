#ifndef ENTRY_H
#define ENTRY_H

#include <windows.h>

namespace Dirt
{
  namespace Entry
  {
    WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries);
    bool getFullPath(char *out, char* relPath, size_t outLen);
    bool removeEntryFromSelection(char *path);
    char **getSelection(int &amountOut);
    void freeSelection(char **selection, int amount);
    void printSelection();
    bool moveSelection();
    bool deleteSelection();
    bool clearAllSelection();
  }
}

#endif // ENTRY_H
