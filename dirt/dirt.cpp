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

using namespace Dirt;

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
