#include <dirt/state/state.h>
#include <dirt/input/input.h>
#include <dirt/rendering/rendering.h>
#include <dirt/context/context.h>

namespace Dirt
{
  namespace State
  {
    void stateMain(Context *context)
    {
      Rendering::renderSplitBox(*context->currentScreen, context->viewsSplitBox);
      Rendering::renderScreenViews(*context->currentScreen);
      Rendering::styleScreenViews(context, *context->currentScreen);
      /* Rendering::rednerTabsContainer(context, */ 
      Rendering::swapScreenBuffers(*(context->currentScreen));
      Input::handleInput(context, *(context->currentScreen), context->stdinHandle);
    }

    void stateVolume(Context *context)
    {

    }

    void statePrompt(Context *context)
    {

    }
  } // namespace State
} // namespace Dirt
