#ifndef SPLITBOX_H
#define SPLITBOX_H

#include <windows.h>
#include <stdint.h>
#include <dirt/structures/container.h>

#define DIRT_SPLIT_NONE 0
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
      char *splitHorizontalLeft;
      char *splitHorizontalRight;
      char *splitVerticalTop;
      char *splitVerticalBottom;
      char *splitCross;
    };

    struct SplitBox
    {
      Container container = {};

      uint8_t nSplits = 0;

      BoxGlyphs glyphs = {};

      uint8_t splitType = DIRT_SPLIT_NONE;

      SplitBox *parent = 0;
      SplitBox *childA = 0; // Top or Left
      SplitBox *childB = 0; // Bottom or Right
    };

    SplitBox *createSplitBox(Structures::Container container, BoxGlyphs glyphs);
    void destroySplitBox(SplitBox *splitBox, uint8_t &splitDeleteCounter);
    void incrementSplitCounter(SplitBox *splitBox);
    void addSplit(SplitBox *splitBox, uint8_t splitType, int32_t signedOffsetAlongOrthogonalAxis, BoxGlyphs *newGlyphs);
  }
}


#endif // SPLITBOX_H
