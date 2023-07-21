#ifndef ENTRY_H
#define ENTRY_H

#include <windows.h>
#include <dirt/context/context.h>

namespace Dirt
{
  namespace Entry
  {
    WIN32_FIND_DATA *findDirectoryEntries(Context *context, char *dirPath, size_t &nEntries);
    bool getFullPath(char *out, char* relPath, size_t outLen);
    bool removeEntryFromSelection(Context *, char *path);
    char **getSelection(Context *context, int &amountOut);
    void freeSelection(char **selection, int amount);
    void printSelection(Context *context);
    bool moveSelection(Context *context);
    bool deleteSelection(Context *context);
    bool clearAllSelection(Context *context);
  }
}

#endif // ENTRY_H
