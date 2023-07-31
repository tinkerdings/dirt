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

    Structures::Container viewBoxContainer = {};
    viewBoxContainer.pos[0] = 1;
    viewBoxContainer.pos[1] = 5;
    uint16_t consoleWidth = 0;
    uint16_t consoleHeight = 0;
    Screen::getConsoleDimensions(consoleWidth, consoleHeight);
    viewBoxContainer.width = consoleWidth - (viewBoxContainer.pos[0]+2);
    viewBoxContainer.height = consoleHeight - (viewBoxContainer.pos[1]+2);

    context->viewBox = Structures::createSplitBox(viewBoxContainer, context->standardGlyphs);
    addSplit(context->viewBox, DIRT_SPLIT_VERTICAL, viewBoxContainer.width/2, 0);
    addSplit(context->viewBox->childA, DIRT_SPLIT_HORIZONTAL, -2, 0);
    addSplit(context->viewBox->childB, DIRT_SPLIT_HORIZONTAL, -2, 0);

    context->entryBufferNSlots = DIRT_ENTRYBUFFER_SIZE;
    context->viewsContainer.pos[0] = viewBoxContainer.pos[0]+1;
    context->viewsContainer.pos[1] = viewBoxContainer.pos[1]+1;
    context->viewsContainer.width = viewBoxContainer.width - 2;
    context->viewsContainer.height = viewBoxContainer.height - 2;

    return context;
  }
}

