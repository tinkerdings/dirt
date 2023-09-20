#include <dirt/context/context.h>
#include <dirt/screen/screen.h>

namespace Dirt
{
  Context *createContext()
  {
    Context *context = (Context *)calloc(1, sizeof(Context));
    if(!context)
    {
      printf("Failed to alloc context\n");
      return 0;
    }

    context->state = DIRT_STATE_MAIN;

    context->stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    if(context->stdinHandle == INVALID_HANDLE_VALUE)
    {
      printf("GetStdHandle failed (%lu)\n", GetLastError());
      return 0;
    }

    SetConsoleMode(context->stdinHandle, ENABLE_WINDOW_INPUT);

    context->standardGlyphs.horizontal                /* = "━";*/  = "═";
    context->standardGlyphs.vertical                  /* = "┃"; */ = "║";
    context->standardGlyphs.topLeft                   /* = "╭"; */ = "╔";
    context->standardGlyphs.topRight                  /* = "╮"; */ = "╗";
    context->standardGlyphs.bottomLeft                /* = "╯"; */ = "╚";
    context->standardGlyphs.bottomRight               /* = "╰"; */ = "╝";
    context->standardGlyphs.splitVerticalTop         /* = "┣"; */ = "╦";
    context->standardGlyphs.splitVerticalBottom        /* = "┫"; */ = "╩";
    context->standardGlyphs.splitHorizontalLeft        /* = "┳"; */ = "╠";
    context->standardGlyphs.splitHorizontalRight     /* = "┻"; */ = "╣";
    context->standardGlyphs.splitCross                /* = "╋"; */ = "╬";

    Structures::Container viewsSplitBoxFrameContainer = {};
    viewsSplitBoxFrameContainer.pos[0] = 1;
    viewsSplitBoxFrameContainer.pos[1] = 5;
    uint16_t consoleWidth = 0;
    uint16_t consoleHeight = 0;
    Screen::getConsoleDimensions(consoleWidth, consoleHeight);
    viewsSplitBoxFrameContainer.width = consoleWidth - 2;
    viewsSplitBoxFrameContainer.height = consoleHeight - viewsSplitBoxFrameContainer.pos[1] - 1;

    context->viewsSplitBox = Structures::createSplitBox(viewsSplitBoxFrameContainer, context->standardGlyphs, "DIRECTORY_TAMER");
    addSplit(context->viewsSplitBox, DIRT_SPLIT_VERTICAL, viewsSplitBoxFrameContainer.width/2, 0);
    addSplit(context->viewsSplitBox->childA, DIRT_SPLIT_HORIZONTAL, -2, 0);
    addSplit(context->viewsSplitBox->childB, DIRT_SPLIT_HORIZONTAL, -2, 0);

    context->entryBufferNSlots = DIRT_ENTRYBUFFER_SIZE;

    return context;
  }
}

