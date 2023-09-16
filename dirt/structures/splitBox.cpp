#include <dirt/structures/splitBox.h>
#include <dirt/error/errorCode.h>

namespace Dirt
{
  namespace Structures
  {
    SplitBox *createSplitBox(Structures::Container frameContainer, BoxGlyphs glyphs)
    {
      SplitBox *splitBox = 0;
      if(!(splitBox = (SplitBox *)calloc(1, sizeof(SplitBox))))
      {
        return 0;
      }
      splitBox->glyphs = glyphs;
      splitBox->frameContainer = frameContainer;

      Structures::Container contentContainer = {};
      contentContainer.pos[0] = frameContainer.pos[0] + 1;
      contentContainer.pos[1] = frameContainer.pos[1] + 1;
      contentContainer.width = frameContainer.width - 1;
      contentContainer.height = frameContainer.height - 1;
      splitBox->contentContainer = contentContainer;

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
          Structures::Container childFrameContainerA;
          childFrameContainerA.pos[0] = splitBox->frameContainer.pos[0];
          childFrameContainerA.pos[1] = splitBox->frameContainer.pos[1];
          childFrameContainerA.width = splitBox->frameContainer.width;
          if(signedOffsetAlongOrthogonalAxis < 0)
          {
            // TODO: This height value does not seem right
            childFrameContainerA.height = splitBox->frameContainer.height + signedOffsetAlongOrthogonalAxis;
          }
          else 
          {
            childFrameContainerA.height = signedOffsetAlongOrthogonalAxis;
          }

          Structures::Container childFrameContainerB;
          childFrameContainerB.pos[0] = splitBox->frameContainer.pos[0];
          childFrameContainerB.pos[1] = splitBox->frameContainer.pos[1] + childFrameContainerA.height;
          childFrameContainerB.width = splitBox->frameContainer.width;
          childFrameContainerB.height = splitBox->frameContainer.height - childFrameContainerA.height;

          splitBox->childA = createSplitBox(childFrameContainerA, childGlyphs);
          splitBox->childA->parent = splitBox;

          splitBox->childB = createSplitBox(childFrameContainerB, childGlyphs);
          splitBox->childB->parent = splitBox;

          break;
        } 

        case(DIRT_SPLIT_VERTICAL):
        {
          Structures::Container childFrameContainerA;
          childFrameContainerA.pos[0] = splitBox->frameContainer.pos[0];
          childFrameContainerA.pos[1] = splitBox->frameContainer.pos[1];
          childFrameContainerA.height = splitBox->frameContainer.height;
          if(signedOffsetAlongOrthogonalAxis < 0)
          {
            childFrameContainerA.width = splitBox->frameContainer.width + signedOffsetAlongOrthogonalAxis;
          }
          else
          {
            childFrameContainerA.width = signedOffsetAlongOrthogonalAxis;
          }

          Structures::Container childFrameContainerB;
          childFrameContainerB.pos[0] = splitBox->frameContainer.pos[0] + childFrameContainerA.width;
          childFrameContainerB.pos[1] = splitBox->frameContainer.pos[1];
          childFrameContainerB.width = splitBox->frameContainer.width - childFrameContainerA.width;
          childFrameContainerB.height = splitBox->frameContainer.height;

          splitBox->childA = createSplitBox(childFrameContainerA, childGlyphs);
          splitBox->childA->parent = splitBox;
          splitBox->childB = createSplitBox(childFrameContainerB, childGlyphs);
          splitBox->childB->parent = splitBox;

          break;
        } 
      }
    }
  } // namespace Structures
} // namespace Dirt
