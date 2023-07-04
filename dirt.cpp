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
  DirectoryView leftView, rightView;
};

struct GlobalState
{
  Screen *currentScreen = 0;
  bool quit = false;
} globalState;

WIN32_FIND_DATA *findDirectoryEntries(char *dirPath, size_t &nEntries);
bool allocScreen(Screen &screen);
bool initScreenDirectoryViews(Screen &screen);
void renderScreenDirectoryViews(Screen &screen);
void renderDirectoryView(Screen &screen, DirectoryView &view);
void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
void styleView(HANDLE screenBuffer, DirectoryView view);
void swapScreenBuffers(Screen &screen);
void handleInput(Screen &screen, HANDLE stdinHandle);

int main(int argc, char **argv)
{
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
    styleView(globalState.currentScreen->backBuffer, globalState.currentScreen->leftView);
    styleView(globalState.currentScreen->backBuffer, globalState.currentScreen->rightView);
    swapScreenBuffers(*globalState.currentScreen);
    handleInput(*globalState.currentScreen, stdinHandle);
  }

  free(firstScreen.leftView.entries);
  free(firstScreen.rightView.entries);

  return 0;
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
    switch(inputBuffer[i].EventType)
    {
      case(KEY_EVENT):
      {
        switch(inputBuffer[i].Event.KeyEvent.wVirtualKeyCode)
        {
          case(VK_ESCAPE):
          case(VK_Q):
          {
            globalState.quit = true;
            return;
          } break;
        }
      } break;
    }
  }
}

bool initScreenDirectoryViews(Screen &screen)
{
  const char *leftPath = ".//*";
  const char *rightPath = "..//*";

  strcpy(screen.leftView.path, leftPath);
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

  strcpy(screen.rightView.path, rightPath);
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

  nEntries = --i;

  return entries;
}
