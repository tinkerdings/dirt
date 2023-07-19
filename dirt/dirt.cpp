#include <windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <winnt.h>
#include <winuser.h>

#include <dirt/context/context.h>
#include <dirt/structures/hashmap.h>
#include <dirt/error/errorCode.h>
#include <dirt/memory/memory.h>
#include <dirt/input/input.h>
#include <dirt/screen/screen.h>

#define DIRT_SELECTIONBUF_MIN_DUPES 10
#define DIRT_SELECTIONBUF_MIN_SIZE 512
#define DIRT_CURSORINDICES_MIN_DUPES 10
#define DIRT_CURSORINDICES_MIN_SIZE 32
#define INPUTBUF_SIZE 3

using namespace Dirt::Context;
using namespace Dirt::Memory;
using namespace Dirt::Error;
using namespace Dirt::Structures;
using namespace Dirt::Input;
using namespace Dirt::Screen;

WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries);
bool initScreenDirectoryViews(ScreenData &screen);
void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
void styleScreenViews(ScreenData &screen);
void incrementScreenCursorIndex(ScreenData &screen);
void decrementScreenCursorIndex(ScreenData &screen);
void setViewPath(DirectoryView &view, char *relPath);
bool getFullPath(char *out, char* relPath, size_t outLen);
bool removeEntryFromSelection(char *path);
bool inputNoKeyRepeat(INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size);
char **getSelection(int &amountOut);
void freeSelection(char **selection, int amount);
void printSelection();
bool moveSelection();
bool deleteSelection();
bool clearAllSelection();
size_t getViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut);

int main(int argc, char **argv)
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

  Context *context = (Context *)malloc(sizeof(Context));
  if(!context)
  {
    printf("Failed to alloc context\n");
    return 1;
  }

  ScreenData firstScreen;
  if(!allocScreen(firstScreen))
  {
    printf("Failed to allocate console screen buffer (%lu)\n", GetLastError());
    return 1;
  }
  context->currentScreen = &firstScreen;

  if(!(context->selection = 
    hashmapCreate(
    DIRT_SELECTIONBUF_MIN_SIZE, 
    DIRT_SELECTIONBUF_MIN_DUPES, 
    MAX_PATH)))
  {
    printf("Failed to init hashmap for entry selection\n");
    return 1;
  }

  initScreenDirectoryViews(firstScreen);

  HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
  if(stdinHandle == INVALID_HANDLE_VALUE)
  {
    printf("GetStdHandle failed (%lu)\n", GetLastError());
    return 1;
  }

  SetConsoleMode(stdinHandle, ENABLE_WINDOW_INPUT);
  while(!context->quit)
  {
    renderScreenDirectoryViews(*context->currentScreen);
    styleScreenViews(*context->currentScreen);
    swapScreenBuffers(*context->currentScreen);
    handleInput(*context->currentScreen, stdinHandle);
  }

  // This is super slow, maybe not a point to do this, since exit.
  /* hashmapDestroy(context->selection); */
  /* hashmapDestroy(context->dirCursorIndices); */
  free(firstScreen.leftView.entries);
  free(firstScreen.rightView.entries);

  return 0;
}


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

char **getSelection(int &amountOut)
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
            errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
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

bool deleteSelection()
{
  int nSelected = 0;
  char **selection = getSelection(nSelected);

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

bool moveSelection()
{
  int nSelected = 0;
  char **selection = getSelection(nSelected);

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

void freeSelection(char **selection, int amount)
{
  for(int i = 0; i < amount; i++)
  {
    free(selection[i]);
  }

  free(selection);
  selection = 0;
}

bool removeEntryFromSelection(char *path)
{
  if(!hashmapRemove(context->selection, path, MAX_PATH))
  {
    printf("hashmapRemove failed\n");
    return false;
  }

  return true;
}

bool clearAllSelection()
{
  int nSelected = 0;
  char **selection = getSelection(nSelected);

  if(!selection)
  {
    printf("Failed to retrieve selection\n");
    return false;
  }

  for(int i = 0; i < nSelected; i++)
  {
    if(!removeEntryFromSelection(selection[i]))
    {
      return false;
    }
  }

  return true;
}

void printSelection()
{
  int nSelected = 0;
  char **selection = getSelection(nSelected);
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

bool inputNoKeyRepeat(INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size)
{
  WORD keyCode = inputBuffer[index].Event.KeyEvent.wVirtualKeyCode;

  if(keyCode == context->input.prevKeyCode)
  {
    return false;
  }

  return true;
}

void setViewPath(DirectoryView &view, char *relPath)
{
  char fullPath[MAX_PATH] = {0};
  getFullPath(fullPath, relPath, MAX_PATH);

  if(!SetCurrentDirectory(fullPath))
  {
    printf("Failed to set current directory (%lu)\n", GetLastError());
  }

  if(view.path[0])
  {
    size_t cursorIndex = view.cursorIndex;
    DirectoryView::CursorMapEntry cursorIndexEntry;
    size_t viewPathLen = strlen(view.path);
    cursorIndexEntry.cursorIndex = cursorIndex;
    memcpy(cursorIndexEntry.path, view.path, viewPathLen);

    size_t hashIndex, dupeIndex;
    if(getViewCursorIndex(view, &hashIndex, &dupeIndex))
    {
      hashmapDirectWrite(view.cursorMap, &cursorIndexEntry, hashIndex, dupeIndex, sizeof(DirectoryView::CursorMapEntry));
    }
    else 
    {
      Dirt::Structures::hashmapInsertAtKey(
        view.cursorMap,
        view.path,
        &cursorIndexEntry,
        viewPathLen,
        sizeof(DirectoryView::CursorMapEntry));
    }
  }

  strcpy(view.path, fullPath);

  free(view.entries);
  view.entries = 0;
  context->maxEntriesInView = 128;
  view.entries = findDirectoryEntries(view.path, view.nEntries);

  view.cursorIndex = getViewCursorIndex(view, 0, 0);
}

void incrementScreenCursorIndex(Screen &screen)
{
  uint32_t currentIndex = screen.active->cursorIndex;
  if(currentIndex < (screen.active->nEntries-1))
  {
    screen.active->cursorIndex++;
  }
  else 
  {
    screen.active->cursorIndex = 0;
  }
}
void decrementScreenCursorIndex(Screen &screen)
{
  uint32_t currentIndex = screen.active->cursorIndex;
  if(currentIndex > 0)
  {
    screen.active->cursorIndex--;
  }
  else 
  {
    screen.active->cursorIndex = screen.active->nEntries-1;
  }
}

size_t getViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut)
{
  size_t viewPathLen = strlen(view.path);
  size_t hashIndex = hashmapGetIndex(view.cursorMap, view.path, viewPathLen);

  if(view.cursorMap->map[hashIndex][0].isSet)
  {
    for(size_t i = 0; i < view.cursorMap->nDupes; i++)
    {
      DirectoryView::CursorMapEntry *entry = (DirectoryView::CursorMapEntry *)view.cursorMap->map[hashIndex][i].data;
      bool ret;
      size_t entryPathLen = strlen(entry->path);
      size_t viewPathLen = strlen(view.path);
      if((ret = Dirt::Memory::compareBytes(entry->path, view.path, entryPathLen, viewPathLen)))
      {
        if(hashIndexOut)
        {
          *hashIndexOut = hashIndex;
        }
        if(dupeIndexOut)
        {
          *dupeIndexOut = i;
        }
        return entry->cursorIndex;
      }
    }
  }

  return 0;
}

bool initScreenDirectoryViews(Screen &screen)
{
  char currentDir[MAX_PATH] = {0};
  if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
  {
    printf("initScreenDirectoryViews:GetCurrentDirectory failed (%lu)\n", GetLastError());
    return false;
  }
  screen.leftView.cursorMap = 
    hashmapCreate(
      DIRT_CURSORINDICES_MIN_SIZE,
      DIRT_CURSORINDICES_MIN_DUPES,
      sizeof(DirectoryView::CursorMapEntry));
  strcpy(screen.leftView.path, currentDir);
  screen.leftView.renderRect.Top = 0;
  screen.leftView.renderRect.Left = 0;
  screen.leftView.renderRect.Bottom = 80;
  screen.leftView.renderRect.Right = 40;
  screen.leftView.width = screen.leftView.renderRect.Right - screen.leftView.renderRect.Left;
  screen.leftView.height = screen.leftView.renderRect.Bottom - screen.leftView.renderRect.Top;
  screen.leftView.entries = findDirectoryEntries(screen.leftView.path, screen.leftView.nEntries);
  if(!screen.leftView.entries)
  {
    printf("findDirectoryEntries failed (%lu)\n", GetLastError());
    return false;
  }

  screen.rightView.cursorMap = 
    hashmapCreate(
        DIRT_CURSORINDICES_MIN_SIZE,
        DIRT_CURSORINDICES_MIN_DUPES,
        sizeof(DirectoryView::CursorMapEntry));
  strcpy(screen.rightView.path, "C:\\");
  screen.rightView.renderRect.Top = 0;
  screen.rightView.renderRect.Left = 42;
  screen.rightView.renderRect.Bottom = 80;
  screen.rightView.renderRect.Right = 82;
  screen.rightView.width = screen.rightView.renderRect.Right - screen.rightView.renderRect.Left;
  screen.rightView.height = screen.rightView.renderRect.Bottom - screen.rightView.renderRect.Top;
  screen.rightView.entries = findDirectoryEntries(screen.rightView.path, screen.rightView.nEntries);
  if(!screen.rightView.entries)
  {
    printf("findDirectoryEntries failed (%lu)\n", GetLastError());
    return false;
  }

  setActiveView(screen, screen.leftView);

  return true;
}

void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory)
{
  size_t filenameLength = strlen(filename);
  int i = 0;
  for(; i < filenameLength; i++)
  {
    CHAR_INFO ci = {(WCHAR)filename[i], FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
    buffer[i] = ci;
  }
  if(isDirectory)
  {
    CHAR_INFO ci = {'/', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
    buffer[i] = ci;
  }
}


// Searches specified directory for files and directories,
// returns number of entries found, and stores entries in <entries> argument
WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries)
{
  if((strlen(dirPath)+3) > MAX_PATH)
  {
    printf("path length exceeds limit\n");
    return 0;
  }
  HANDLE entry = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA *entries = (WIN32_FIND_DATA *)malloc(context->maxEntriesInView * sizeof(WIN32_FIND_DATA));
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
    while((i < context->maxEntriesInView) && (findSuccess = FindNextFile(entry, &entries[i++]))){}
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
      context->maxEntriesInView *= 2;
      WIN32_FIND_DATA *tmpPtr = (WIN32_FIND_DATA *)realloc(entries, context->maxEntriesInView);
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
