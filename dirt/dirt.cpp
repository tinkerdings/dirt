#include "structures/hashmap.h"
#include <windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <winnt.h>
#include <winuser.h>

#include <dirt/structures/hashmap.h>
#include <dirt/error/errorCode.h>
#include <dirt/memory/memory.h>

#define DIRT_SELECTIONBUF_MIN_DUPES 10
#define DIRT_SELECTIONBUF_MIN_SIZE 512
#define DIRT_CURSORINDICES_MIN_DUPES 10
#define DIRT_CURSORINDICES_MIN_SIZE 32
#define INPUTBUF_SIZE 3

#define VK_Q 0x51
#define VK_H 0x48
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_D 0x44

using namespace Dirt::Memory;
using namespace Dirt::Error;
using namespace Dirt::Structures;

struct DirectoryView
{
  char path[MAX_PATH] = {0};
  SMALL_RECT renderRect = {0};
  SHORT width = 0, height = 0;
  size_t nEntries = 0;
  WIN32_FIND_DATA *entries = 0;
  uint32_t cursorIndex = 0;
  Hashmap *cursorMap;
};

struct Screen
{
  HANDLE backBuffer, frontBuffer;
  DirectoryView leftView, rightView, *active;
};

struct Input
{
  WORD prevKeyCode = -1;
};

struct GlobalState
{
  Screen *currentScreen = 0;
  bool quit = false;
  size_t maxEntriesInView = 128;
  Hashmap *selection;
  Input input;
} globalState;

WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries);
bool allocScreen(Screen &screen);
bool initScreenDirectoryViews(Screen &screen);
void renderScreenDirectoryViews(Screen &screen);
void renderDirectoryView(Screen &screen, DirectoryView &view);
void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
void styleScreenViews(Screen &screen);
void styleView(HANDLE screenBuffer, DirectoryView view);
void highlightLine(Screen &screen);
void swapScreenBuffers(Screen &screen);
void handleInput(Screen &screen, HANDLE stdinHandle);
void incrementScreenCursorIndex(Screen &screen);
void decrementScreenCursorIndex(Screen &screen);
bool setActiveView(Screen &screen, DirectoryView &view);
void setViewPath(DirectoryView &view, char *relPath);
void clearScreen(Screen &screen);
bool getFullPath(char *out, char* relPath, size_t outLen);
bool removeEntryFromSelection(char *path);
bool inputNoKeyRepeat(INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size);
char **getSelection(int &amountOut);
void freeSelection(char **selection, int amount);
void printSelection();
bool moveSelection();
bool deleteSelection();
bool clearAllSelection();

int main(int argc, char **argv)
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  Screen firstScreen;
  if(!allocScreen(firstScreen))
  {
    printf("Failed to allocate console screen buffer (%lu)\n", GetLastError());
    return 1;
  }
  globalState.currentScreen = &firstScreen;

  if(!(globalState.selection = 
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
  char* preselected = "W:\\test\\b\\b - Copy (3).txt";
  hashmapInsert(globalState.selection, preselected, strlen(preselected));
  while(!globalState.quit)
  {
    renderScreenDirectoryViews(*globalState.currentScreen);
    styleScreenViews(*globalState.currentScreen);
    swapScreenBuffers(*globalState.currentScreen);
    handleInput(*globalState.currentScreen, stdinHandle);
  }

  // This is super slow, maybe not a point to do this, since exit.
  /* hashmapDestroy(globalState.selection); */
  /* hashmapDestroy(globalState.dirCursorIndices); */
  free(firstScreen.leftView.entries);
  free(firstScreen.rightView.entries);

  return 0;
}


bool getFullPath(char *out, char* relPath, size_t outLen)
{
  char currentDir[MAX_PATH] = {0};
  GetCurrentDirectoryA(MAX_PATH, currentDir);
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

void clearScreen(Screen &screen)
{
  CHAR_INFO leftClear[MAX_PATH] = {0};
  for(int i = 0; i < screen.leftView.width; i++)
  {
    leftClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
  }
  for(int i = 0; i < screen.leftView.nEntries; i++)
  {
    COORD size {screen.leftView.width, 1};
    COORD pos = {0, 0};
    SMALL_RECT rect;
    rect.Top = screen.leftView.renderRect.Top+i;
    rect.Left = screen.leftView.renderRect.Left;
    rect.Bottom = screen.leftView.renderRect.Bottom;
    rect.Right = screen.leftView.renderRect.Right;
    if(!WriteConsoleOutput(screen.backBuffer, leftClear, size, pos, &rect))
    {
      printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
      return;
    }
    if(!WriteConsoleOutput(screen.frontBuffer, leftClear, size, pos, &rect))
    {
      printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
      return;
    }
  }
  CHAR_INFO rightClear[MAX_PATH] = {0};
  for(int i = 0; i < screen.rightView.width; i++)
  {
    rightClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
  }
  for(int i = 0; i < screen.rightView.nEntries; i++)
  {
    COORD size {screen.rightView.width, 1};
    COORD pos = {0, 0};
    SMALL_RECT rect;
    rect.Top = screen.rightView.renderRect.Top+i;
    rect.Left = screen.rightView.renderRect.Left;
    rect.Bottom = screen.rightView.renderRect.Bottom;
    rect.Right = screen.rightView.renderRect.Right;
    if(!WriteConsoleOutput(screen.backBuffer, rightClear, size, pos, &rect))
    {
      printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
      return;
    }
    if(!WriteConsoleOutput(screen.frontBuffer, rightClear, size, pos, &rect))
    {
      printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
      return;
    }
  }
}

char **getSelection(int &amountOut)
{
  int bufSize = globalState.selection->nSlots;
  char **selected = 0;
  if(!(selected = (char **)calloc(bufSize, sizeof(char *))))
  {
    printf("calloc failed to allocate array of strings for selected entries\n");
    return 0;
  }
  for(int i = 0; i < globalState.selection->nSlots; i++)
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

  for(int i = 0; i < globalState.selection->nSlots; i++)
  {
    if(globalState.selection->map[i][0].isSet)
    {
      for(int j = 0; j < globalState.selection->nDupes; j++)
      {
        if(!globalState.selection->map[i][j].isSet)
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
            for(int k = idx; k < globalState.selection->nSlots; k++)
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

        memcpy(selected[idx], globalState.selection->map[i][j].data, globalState.selection->map[i][j].nBytes);
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
  if(!hashmapRemove(globalState.selection, path, MAX_PATH))
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
void handleInput(Screen &screen, HANDLE stdinHandle)
{
  INPUT_RECORD inputBuffer[INPUTBUF_SIZE] = {0};
  DWORD nRecordsRead = 0;
  if(!ReadConsoleInput(
    stdinHandle, inputBuffer, 3, &nRecordsRead))
  {
    printf("ReadConsoleInput failed (%lu)\n", GetLastError());
    return;
  }

  for(int i = 0; i < nRecordsRead; i++)
  {
    WIN32_FIND_DATA &activeEntry = screen.active->entries[screen.active->cursorIndex];
    switch(inputBuffer[i].EventType)
    {
      case(KEY_EVENT):
      {
        if(!inputBuffer[i].Event.KeyEvent.bKeyDown)
        {
          globalState.input.prevKeyCode = -1;
          break;
        }
        switch(inputBuffer[i].Event.KeyEvent.wVirtualKeyCode)
        {
          case(VK_ESCAPE):
          case(VK_Q):
          {
            globalState.quit = true;
            return;
          } break;
          case(VK_DOWN):
          case(VK_J):
          {
            incrementScreenCursorIndex(screen);
          } break;
          case(VK_UP):
          case(VK_K):
          {
            decrementScreenCursorIndex(screen);
          } break;
          case(VK_TAB):
          {
            if(screen.active == &screen.leftView)
            {
              screen.active = &screen.rightView;
            }
            else 
            {
              screen.active = &screen.leftView;
            }
            if(!SetCurrentDirectory(screen.active->path))
            {
              printf("Failed to set current directory (%lu)\n", GetLastError());
            }
          } break;
          case(VK_RETURN):
          case(VK_RIGHT):
          case(VK_L):
          {
            if(activeEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
              clearScreen(*globalState.currentScreen);
              setViewPath(*screen.active, activeEntry.cFileName);
            }
            else
            {
              char fullPath[MAX_PATH] = {0};
              if(!getFullPath(fullPath, activeEntry.cFileName, MAX_PATH))
              {
                printf("fullPath failed (%lu)\n", GetLastError());
                break;
              }
              printf("fullPath: %s\n", fullPath);
              DWORD binaryType = -1;
              // TODO: Error handling for file opening
              if(GetBinaryTypeA(fullPath, &binaryType))
              {
                if(binaryType == SCS_32BIT_BINARY || binaryType == SCS_64BIT_BINARY)
                {
                  HANDLE exeHandle = CreateFileA(
                    fullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                  if(exeHandle == INVALID_HANDLE_VALUE)
                  {
                    printf("CreateFileA failed (%lu)\n", GetLastError());
                    break;
                  }
                  HANDLE exeMapping = CreateFileMappingA(
                    exeHandle,
                    0,
                    PAGE_READONLY, 0, 0, 0);
                  if(!exeMapping || exeMapping == INVALID_HANDLE_VALUE)
                  {
                    printf("CreateFileMappingA failed (%lu)\n", GetLastError());
                    break;
                  }
                  void* exe = MapViewOfFile(
                    exeMapping, FILE_MAP_READ, 0, 0, 0);
                  PIMAGE_NT_HEADERS ntHeaders = ImageNtHeader(exe);
                  switch(ntHeaders->OptionalHeader.Subsystem)
                  {
                    case(IMAGE_SUBSYSTEM_WINDOWS_CUI):
                    {
                      ShellExecuteA(0, 0, fullPath, 0, 0, SW_SHOW);
                    } break;
                    case(IMAGE_SUBSYSTEM_WINDOWS_GUI):
                    {
                      PROCESS_INFORMATION exeInfo;
                      STARTUPINFOA startupInfo = {
                        sizeof(STARTUPINFOA),
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        STARTF_USESHOWWINDOW, SW_SHOW,
                        0, 0, 0, 0, 0
                      };
                      CreateProcessA(fullPath, 
                        0, 0, 0, false,
                        NORMAL_PRIORITY_CLASS,
                        0, 0,
                        &startupInfo, &exeInfo);
                    } break;
                  }
                }
              }
              else 
              {
                if(ShellExecuteA(0, 0, fullPath, 0, 0, SW_SHOW) <= (HINSTANCE)32)
                {
                  printf("ShellExecuteA failed (%lu)\n", GetLastError());
                  break;
                }
              }
            }
          } break;
          case(VK_BACK):
          case(VK_LEFT):
          case(VK_H):
          {
            clearScreen(*globalState.currentScreen);
            setViewPath(*screen.active, "..");
          } break;
          case(VK_SPACE):
          {
            if(inputBuffer[i].Event.KeyEvent.dwControlKeyState == SHIFT_PRESSED)
            {
              clearAllSelection();
              break;
            }
            if(inputNoKeyRepeat(inputBuffer, i, INPUTBUF_SIZE))
            {
              char fullPath[MAX_PATH] = {0};
              if(!getFullPath(fullPath, activeEntry.cFileName, MAX_PATH))
              {
                printf("getFullPath failed (%lu)\n", GetLastError());
                break;
              }
              if(hashmapContains(globalState.selection, fullPath, MAX_PATH, 0, 0))
              {
                removeEntryFromSelection(fullPath);
              }
              else 
              {
                int ret = hashmapInsert(globalState.selection, fullPath, MAX_PATH);
                if(ret == DIRT_ERROR_ALLOCATION_FAILURE)
                {
                  printf("hashMapInsert failed with error DIRT_SEL_ALLOCATION_FAILURE (0x1)\n");
                  break;
                }
                if(ret == DIRT_ERROR_INVALID_ENTRY)
                {
                  printf("hashMapInsert failed with error DIRT_SEL_INVALID_ENTRY (0x2), for path: %s\n", fullPath);
                  break;
                }
              }
            }
          } break;
          case(VK_M):
          {
            moveSelection();
            clearAllSelection();
          } break;
          case(VK_D):
          {
            deleteSelection();
            clearAllSelection();
          } break;
        }

        globalState.input.prevKeyCode = inputBuffer[i].Event.KeyEvent.wVirtualKeyCode;
      } break;
    }
  }
}

bool inputNoKeyRepeat(INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size)
{
  WORD keyCode = inputBuffer[index].Event.KeyEvent.wVirtualKeyCode;

  if(keyCode == globalState.input.prevKeyCode)
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

  uint32_t cursorIndex = view.cursorIndex;
  size_t hashIndex = hashmapGetIndex(view.cursorMap, view.path, MAX_PATH);
  Dirt::Structures::hashmapDirectWrite(view.cursorMap, &cursorIndex, hashIndex, 0, sizeof(uint32_t));

  strcpy(view.path, fullPath);

  free(view.entries);
  view.entries = 0;
  globalState.maxEntriesInView = 128;
  view.entries = findDirectoryEntries(view.path, view.nEntries);

  hashIndex = hashmapGetIndex(view.cursorMap, view.path, MAX_PATH);
  if(view.cursorMap->map[hashIndex][0].isSet)
  {
    view.cursorIndex = (uint32_t)(*view.cursorMap->map[hashIndex][0].data);
  }
  else 
  {
    view.cursorIndex = 0;
  }
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

bool initScreenDirectoryViews(Screen &screen)
{
  char currentDir[MAX_PATH] = {0};
  if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
  {
    printf("initScreenDirectoryViews:GetCurrentDirectory failed (%lu)\n", GetLastError());
    return false;
  }
  screen.leftView.cursorMap = hashmapCreate(DIRT_CURSORINDICES_MIN_SIZE, DIRT_CURSORINDICES_MIN_DUPES, 1);
  setViewPath(screen.leftView, "W:\\test\\b"); // TODO set this to currentDir, this is just for testing.
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

  screen.rightView.cursorMap = hashmapCreate(DIRT_CURSORINDICES_MIN_SIZE, DIRT_CURSORINDICES_MIN_DUPES, 1);
  setViewPath(screen.rightView, "C:\\");
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

bool setActiveView(Screen &screen, DirectoryView &view)
{
  screen.active = &view;
  if(!SetCurrentDirectory(screen.active->path))
  {
    printf("Failed to set current directory (%lu)\n", GetLastError());
    return false;
  }
  return true;
}

bool allocScreen(Screen &screen)
{
  screen.backBuffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL);
  screen.frontBuffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL);

  if((screen.backBuffer == INVALID_HANDLE_VALUE) || (screen.frontBuffer == INVALID_HANDLE_VALUE))
  {
    return false;
  }

  return true;
}

void renderScreenDirectoryViews(Screen &screen)
{
  renderDirectoryView(screen, screen.leftView);
  renderDirectoryView(screen, screen.rightView);
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

void styleView(HANDLE screenBuffer, DirectoryView view)
{
  for(int i = 0; i < view.nEntries; i++)
  {
    WIN32_FIND_DATA entry = view.entries[i];

    WORD attributes = entry.dwFileAttributes;
    
    COORD coords;
    coords.X = view.renderRect.Left;
    coords.Y = i;
    size_t filenameLength = strlen(view.entries[i].cFileName);
    DWORD nSet = 0;

    if(attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
    FillConsoleOutputAttribute(
        screenBuffer,
        FOREGROUND_RED | FOREGROUND_GREEN,
        filenameLength+1,
        coords,
        &nSet);
    }

    /* char currentDir[MAX_PATH] = {0}; */
    /* GetCurrentDirectoryA(MAX_PATH, currentDir); */
    /* if(!SetCurrentDirectory(view.path)) */
    /* { */
    /*   printf("Failed to set current directory (%lu)\n", GetLastError()); */
    /*   return; */
    /* } */
    char fullPath[MAX_PATH] = {0};
    getFullPath(fullPath, entry.cFileName, MAX_PATH);

    if(hashmapContains(globalState.selection, fullPath, strlen(fullPath), 0, 0))
    {
      FillConsoleOutputAttribute(
          screenBuffer,
          FOREGROUND_GREEN,
          filenameLength,
          coords,
          &nSet);
    }
    /* if(!SetCurrentDirectory(currentDir)) */
    /* { */
    /*   printf("Failed to set current directory (%lu)\n", GetLastError()); */
    /*   return; */
    /* } */
  }
}

void highlightLine(Screen &screen)
{
  size_t cursorFilenameLength = strlen(screen.active->entries[screen.active->cursorIndex].cFileName);
  size_t emptySpace = 
    screen.active->width - cursorFilenameLength;
  COORD coords;
  DWORD nSet = 0;
  coords.X = screen.active->renderRect.Left;
  coords.Y = screen.active->cursorIndex;
  WORD attribs = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;

  char fullPath[MAX_PATH] = {0};
  getFullPath(fullPath, screen.active->entries[screen.active->cursorIndex].cFileName, MAX_PATH);

  if(hashmapContains(globalState.selection, fullPath, MAX_PATH, 0, 0))
  {
    attribs ^= FOREGROUND_RED | FOREGROUND_BLUE;
  }

  FillConsoleOutputAttribute(
    screen.backBuffer,
    attribs,
    screen.active->width,
    coords,
    &nSet);

  coords.X += cursorFilenameLength;
  FillConsoleOutputCharacter(screen.backBuffer,
    ' ',
    emptySpace,
    coords,
    &nSet);
}

void styleScreenViews(Screen &screen)
{
  styleView(screen.backBuffer, screen.leftView);
  styleView(screen.backBuffer, screen.rightView);
  highlightLine(screen);
}

void swapScreenBuffers(Screen &screen)
{
  HANDLE temp = screen.frontBuffer;
  screen.frontBuffer = screen.backBuffer;
  screen.backBuffer = temp;
  if(!SetConsoleActiveScreenBuffer(screen.frontBuffer))
  {
    printf("SetConsoleActiveScreenBuffer failed (%lu)\n", GetLastError());
    return;
  }
}

void renderDirectoryView(Screen &screen, DirectoryView &view)
{
  for(int i = 0; i < view.nEntries; i++)
  {
    CHAR_INFO filename[MAX_PATH] = {0};
    bool isDirectory = (view.entries[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    createFilenameCharInfoBuffer(filename, view.entries[i].cFileName, view.width, isDirectory);
    COORD filenameSize {view.width, 1};
    COORD filenamePos = {0, 0};
    SMALL_RECT rect;
    rect.Top = view.renderRect.Top+i;
    rect.Left = view.renderRect.Left;
    rect.Bottom = view.renderRect.Bottom;
    rect.Right = view.renderRect.Right;
    if(!WriteConsoleOutput(screen.backBuffer, filename, filenameSize, filenamePos, &rect))
    {
      printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
      return;
    }
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
  WIN32_FIND_DATA *entries = (WIN32_FIND_DATA *)malloc(globalState.maxEntriesInView * sizeof(WIN32_FIND_DATA));
  char fullPath[MAX_PATH] = {0};
  GetFullPathName(dirPath, MAX_PATH, fullPath, 0);
  strcat(fullPath, "\\*");

  entry = FindFirstFile(fullPath, &entries[0]);
  if(entry == INVALID_HANDLE_VALUE)
  {
    printf("FindFirstFile failed (%lu)\n", GetLastError());
    free(entries);
    return 0;
  }

  size_t i = 1;
  BOOL findSuccess = 1;
  while(1)
  {
    while((i < globalState.maxEntriesInView) && (findSuccess = FindNextFile(entry, &entries[i++]))){}
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
      globalState.maxEntriesInView *= 2;
      WIN32_FIND_DATA *tmpPtr = (WIN32_FIND_DATA *)realloc(entries, globalState.maxEntriesInView);
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
