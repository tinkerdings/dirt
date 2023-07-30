#ifndef SPLITBOX_H
#define SPLITBOX_H

#include <windows.h>
#include <stdint.h>
#include <dirt/structures/container.h>

#define DIRT_SPLIT_HORIZONTAL 1
#define DIRT_SPLIT_VERTICAL 2

namespace Dirt
{
  namespace Structures
  {
    struct BoxGlyphs
    {
      char *horizontal;
      char *vertical;
      char *topLeft;
      char *topRight;
      char *bottomLeft;
      char *bottomRight;
      char *splitHorizontalTop;
      char *splitHorizontalBottom;
      char *splitVerticalLeft;
      char *splitVerticalRight;
      char *splitCross;
    };

    struct SplitBox
    {
      Container container = {0};

      uint8_t nSplits = 0;

      BoxGlyphs glyphs = {0};

      SplitBox *parent = 0;
      SplitBox *left = 0;
      SplitBox *right = 0;
      SplitBox *top = 0;
      SplitBox *bottom = 0;
    };

    SplitBox *createSplitBox(Structures::Container container, BoxGlyphs glyphs);
    void destroySplitBox(SplitBox *splitBox, uint8_t &splitDeleteCounter);
    void incrementSplitCounter(SplitBox *splitBox);
    void addSplit(SplitBox *splitBox, uint8_t splitType, uint16_t offsetAlongOrthogonalAxis, BoxGlyphs *newGlyphs);
  }
}


#endif // SPLITBOX_H
