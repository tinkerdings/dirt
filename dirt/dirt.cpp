#include <windows.h>
#include <shlwapi.h>
#include <DbgHelp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dirt/context/context.h>
#include <dirt/structures/hashmap.h>
#include <dirt/error/errorCode.h>
#include <dirt/memory/memory.h>
#include <dirt/input/input.h>
#include <dirt/screen/screen.h>
#include <dirt/predefinedValues.h>
#include <dirt/rendering/rendering.h>
#include <dirt/structures/splitBox.h>

using namespace Dirt;

int main(int argc, char **argv)
{
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

  Context *context = createContext();
  if(!context)
  {
    return 1;
  }

  if(!initScreens(context, DIRT_N_SCREENS))
  {
    printf("Failed to initialize screens\n");
    return 1;
  }
  Screen::setCurrentScreen(context, 0);

  /* CONSOLE_FONT_INFOEX cfi = */ 
  /* { */
  /*   sizeof(cfi), */
  /*   0, */
  /*   0, 14, */
  /*   FF_DONTCARE, */
  /*   FW_NORMAL, */
  /*   L"unifont" */
  /* }; */
  /* SetCurrentConsoleFontEx(context->currentScreen->backBuffer, false, &cfi); */

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
    Rendering::renderScreenViews(*context->currentScreen, context->viewsContainer);
    Rendering::styleScreenViews(context, *(context->currentScreen));
    Rendering::renderSplitBox(*context->currentScreen, context->viewBox);
    Rendering::highlightLine(context, *(context->currentScreen));
    Rendering::swapScreenBuffers(*(context->currentScreen));
    Input::handleInput(context, *(context->currentScreen), stdinHandle);
  }

  // This is super slow, maybe not a point to do this, since exit.
  /* hashmapDestroy(context->selection); */
  /* hashmapDestroy(context->dirCursorIndices); */
  uint8_t deleteCounter = 0;
  Structures::destroySplitBox(context->viewBox, deleteCounter);
  for(int i = 0; i < DIRT_N_SCREENS; i++)
  {
    free(context->screens[i]->leftView.entries);
    free(context->screens[i]->rightView.entries);
  }
  free(context->screens);
  free(context);

  return 0;
}
