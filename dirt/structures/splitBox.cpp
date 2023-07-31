#include <dirt/structures/splitBox.h>
#include <dirt/error/errorCode.h>

namespace Dirt
{
  namespace Structures
  {
    SplitBox *createSplitBox(Structures::Container container, BoxGlyphs glyphs)
    {
      SplitBox *splitBox = 0;
      if(!(splitBox = (SplitBox *)calloc(1, sizeof(SplitBox))))
      {
        return 0;
      }
      splitBox->glyphs = glyphs;
      splitBox->container = container;

      return splitBox;
    }

    void destroySplitBox(SplitBox *splitBox, uint8_t &splitDeleteCounter)
    {
      if(splitBox->childA)
      {
        destroySplitBox(splitBox->childA, splitDeleteCounter);
        splitDeleteCounter++;
      }
      if(splitBox->childB)
      {
        destroySplitBox(splitBox->childB, splitDeleteCounter);
        splitDeleteCounter++;
      }

      splitDeleteCounter++;

      if(splitBox->parent)
      {
        splitBox->parent->nSplits -= splitDeleteCounter;
      }

      free(splitBox);
      splitBox = 0;
    }

    void incrementSplitCounter(SplitBox *splitBox)
    {
      splitBox->nSplits++;
      if(splitBox->parent)
      {
        incrementSplitCounter(splitBox->parent);
      }
    }

    void addSplit(SplitBox *splitBox, uint8_t splitType, int32_t signedOffsetAlongOrthogonalAxis, BoxGlyphs *newGlyphs)
    {
      if(splitBox->splitType != DIRT_SPLIT_NONE)
      {
        Error::errorCode = DIRT_ERROR_SPLITBOX_ALREADY_SPLIT;
        return;
      }

      incrementSplitCounter(splitBox);
      BoxGlyphs childGlyphs = splitBox->glyphs;
      if(newGlyphs)
      {
        memcpy(&childGlyphs, newGlyphs, sizeof(BoxGlyphs));
      }

      splitBox->splitType = splitType;
      switch(splitType)
      {
        case(DIRT_SPLIT_HORIZONTAL):
        {
          Structures::Container childContainerA;
          childContainerA.pos[0] = splitBox->container.pos[0];
          childContainerA.pos[1] = splitBox->container.pos[1];
          childContainerA.width = splitBox->container.width;
          if(signedOffsetAlongOrthogonalAxis < 0)
          {
            childContainerA.height = splitBox->container.height + signedOffsetAlongOrthogonalAxis;
          }
          else 
          {
            childContainerA.height = signedOffsetAlongOrthogonalAxis;
          }

          Structures::Container childContainerB;
          childContainerB.pos[0] = splitBox->container.pos[0];
          childContainerB.pos[1] = splitBox->container.pos[1] + childContainerA.height;
          childContainerB.width = splitBox->container.width;
          childContainerB.height = splitBox->container.height - childContainerA.height;

          splitBox->childA = createSplitBox(childContainerA, childGlyphs);
          splitBox->childA->parent = splitBox;

          splitBox->childB = createSplitBox(childContainerB, childGlyphs);
          splitBox->childB->parent = splitBox;
        } break;

        case(DIRT_SPLIT_VERTICAL):
        {
          Structures::Container childContainerA;
          childContainerA.pos[0] = splitBox->container.pos[0];
          childContainerA.pos[1] = splitBox->container.pos[1];
          if(signedOffsetAlongOrthogonalAxis < 0)
          {
            childContainerA.width = splitBox->container.width + signedOffsetAlongOrthogonalAxis;
          }
          else
          {
            childContainerA.width = signedOffsetAlongOrthogonalAxis;
          }
          childContainerA.height = splitBox->container.height;

          Structures::Container childContainerB;
          childContainerB.pos[0] = splitBox->container.pos[0] + childContainerA.width;
          childContainerB.pos[1] = splitBox->container.pos[1];
          childContainerB.width = splitBox->container.width - childContainerA.width;
          childContainerB.height = splitBox->container.height;

          splitBox->childA = createSplitBox(childContainerA, childGlyphs);
          splitBox->childA->parent = splitBox;
          splitBox->childB = createSplitBox(childContainerB, childGlyphs);
          splitBox->childB->parent = splitBox;
        } break;
      }
    }
  } // namespace Structures
} // namespace Dirt
