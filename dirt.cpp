#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VK_Q 0x51

#define VK_H 0x48
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C

struct DirectoryView
{
  char path[MAX_PATH] = {0};
  SMALL_RECT renderRect = {0};
  SHORT width = 0, height = 0;
  size_t nEntries = 0;
  WIN32_FIND_DATA *entries = 0;
  uint32_t cursorIndex = 0;
};

struct Screen
{
  HANDLE backBuffer, frontBuffer;
  DirectoryView leftView, rightView, *active;
};

struct GlobalState
{
  Screen *currentScreen = 0;
  bool quit = false;
  size_t maxEntriesInView = 128;
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
void setViewPathRelative(DirectoryView &view, const char *relPath);
void clearScreen(Screen &screen);

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
  initScreenDirectoryViews(firstScreen);

  HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
  if(stdinHandle == INVALID_HANDLE_VALUE)
  {
    printf("GetStdHandle failed (%lu)\n", GetLastError());
    return 1;
  }

  SetConsoleMode(stdinHandle, ENABLE_WINDOW_INPUT);

  while(!globalState.quit)
  {
    renderScreenDirectoryViews(*globalState.currentScreen);
    styleScreenViews(*globalState.currentScreen);
    swapScreenBuffers(*globalState.currentScreen);
    handleInput(*globalState.currentScreen, stdinHandle);
  }

  free(firstScreen.leftView.entries);
  free(firstScreen.rightView.entries);

  return 0;
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
  screen.leftView.cursorIndex = 0;
  screen.rightView.cursorIndex = 0;
}

void handleInput(Screen &screen, HANDLE stdinHandle)
{
  INPUT_RECORD inputBuffer[3] = {0};
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
          case(VK_J):
          {
            incrementScreenCursorIndex(screen);
          } break;
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
          } break;
          case(VK_L):
          {
            printf("activeEntry.dwFileAttributes: %lu\n", activeEntry.dwFileAttributes);
            if(activeEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
              printf("-> directory\n");
              clearScreen(*globalState.currentScreen);
              setViewPathRelative(*screen.active, activeEntry.cFileName);

              if(!SetCurrentDirectory(screen.active->path))
              {
                printf("Failed to set current directory (%lu)\n", GetLastError());
              }
            }
            else
            {
              char fullPath[MAX_PATH] = {0};
              GetFullPathNameA(activeEntry.cFileName, MAX_PATH, fullPath, 0);
              printf("fullPath: %s\n", fullPath);
              ShellExecuteA(0, 0, fullPath, 0, 0, SW_SHOW);
            }
          } break;
          case(VK_H):
          {
              clearScreen(*globalState.currentScreen);
              setViewPathRelative(*screen.active, "..");
          } break;
        }
      } break;
    }
  }
}

void setViewPathRelative(DirectoryView &view, const char *relPath)
{
  if((strlen(view.path)+strlen(relPath)+2) > MAX_PATH)
  {
    printf("path length exceeds limit\n");
    return;
  }
  strcat(view.path, "/");
  strcat(view.path, relPath);

  free(view.entries);
  view.entries = 0;
  globalState.maxEntriesInView = 128;
  view.entries = findDirectoryEntries(view.path, view.nEntries);
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
  strcpy(screen.leftView.path, "C:\\");
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

  strcpy(screen.rightView.path, "D:\\");
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

  screen.active = &screen.leftView;
  if(!SetCurrentDirectory(screen.leftView.path))
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
    WORD attributes = view.entries[i].dwFileAttributes;
    
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
  FillConsoleOutputAttribute(
    screen.backBuffer,
    BACKGROUND_BLUE,
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
      realloc(entries, globalState.maxEntriesInView);
    }
  }

  FindClose(entry);

  nEntries = --i;

  return entries;
}
