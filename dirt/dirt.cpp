#include <windows.h>
#include <shlwapi.h>
#include <DbgHelp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wingdi.h>
#include <winnt.h>
#include <winuser.h>

#include <dirt/context/context.h>
#include <dirt/structures/hashmap.h>
#include <dirt/error/errorCode.h>
#include <dirt/memory/memory.h>
#include <dirt/input/input.h>
#include <dirt/screen/screen.h>
#include <dirt/predefinedValues.h>

using namespace Dirt;

int main(int argc, char **argv)
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

  Context *context = (Context *)calloc(1, sizeof(Context));
  if(!context)
  {
    printf("Failed to alloc context\n");
    return 1;
  }


  context->entryBufferNSlots = DIRT_ENTRYBUFFER_SIZE;
  context->viewsContainer.pos[0] = 2;
  context->viewsContainer.pos[1] = 4;
  context->viewsContainer.width = 100;
  context->viewsContainer.height = 40;

  if(!initScreens(context, DIRT_N_SCREENS))
  {
    printf("Failed to initialize screens\n");
    return 1;
  }
  Screen::setCurrentScreen(context, 0);

  CONSOLE_FONT_INFOEX cfi = 
  {
    sizeof(cfi),
    0,
    0, 14,
    FF_DONTCARE,
    FW_NORMAL,
    L"unifont"
  };
  SetCurrentConsoleFontEx(context->currentScreen->backBuffer, false, &cfi);

  if(!(context->selection = 
    hashmapCreate(
    DIRT_SELECTIONBUF_MIN_SIZE, 
    DIRT_SELECTIONBUF_MIN_DUPES, 
    MAX_PATH)))
  {
    printf("Failed to init hashmap for entry selection\n");
    return 1;
  }

  HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
  if(stdinHandle == INVALID_HANDLE_VALUE)
  {
    printf("GetStdHandle failed (%lu)\n", GetLastError());
    return 1;
  }

  SetConsoleMode(stdinHandle, ENABLE_WINDOW_INPUT);

  while(!context->quit)
  {
    renderScreenViews(*context->currentScreen, context->viewsContainer);
    styleScreenViews(context, *(context->currentScreen));
    highlightLine(context, *(context->currentScreen));
    swapScreenBuffers(*(context->currentScreen));
    Input::handleInput(context, *(context->currentScreen), stdinHandle);
  }

  // This is super slow, maybe not a point to do this, since exit.
  /* hashmapDestroy(context->selection); */
  /* hashmapDestroy(context->dirCursorIndices); */
  for(int i = 0; i < DIRT_N_SCREENS; i++)
  {
    free(context->screens[i]->leftView.entries);
    free(context->screens[i]->rightView.entries);
  }
  free(context->screens);
  free(context);

  return 0;
}
