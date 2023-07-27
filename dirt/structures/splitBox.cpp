#include <dirt/structures/splitBox.h>

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

      return splitBox;
    }

    void destroySplitBox(SplitBox *splitBox, uint8_t &splitDeleteCounter)
    {
      if(splitBox->left)
      {
        destroySplitBox(splitBox->left, splitDeleteCounter);
        splitDeleteCounter++;
      }
      if(splitBox->right)
      {
        destroySplitBox(splitBox->right, splitDeleteCounter);
        splitDeleteCounter++;
      }
      if(splitBox->top)
      {
        destroySplitBox(splitBox->top, splitDeleteCounter);
        splitDeleteCounter++;
      }
      if(splitBox->bottom)
      {
        destroySplitBox(splitBox->bottom, splitDeleteCounter);
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

    void addSplit(SplitBox *splitBox, uint8_t splitType, uint16_t offsetAlongAxis, BoxGlyphs *newGlyphs)
    {
      incrementSplitCounter(splitBox);
      BoxGlyphs childGlyphs = splitBox->glyphs;
      if(newGlyphs)
      {
        memcpy(&childGlyphs, newGlyphs, sizeof(BoxGlyphs));
      }

      switch(splitType)
      {
        case(DIRT_SPLIT_HORIZONTAL):
        {
          Structures::Container topContainer;
          topContainer.pos[0] = splitBox->container.pos[0];
          topContainer.pos[1] = splitBox->container.pos[1];
          topContainer.width = splitBox->container.width;
          topContainer.height = splitBox->container.height/2;

          Structures::Container bottomContainer;
          bottomContainer.pos[0] = splitBox->container.pos[0];
          bottomContainer.pos[1] = splitBox->container.pos[1] + splitBox->container.height/2;
          bottomContainer.width = splitBox->container.width;
          bottomContainer.height = splitBox->container.height/2;

          splitBox->top = createSplitBox(topContainer, childGlyphs);
          splitBox->bottom = createSplitBox(bottomContainer, childGlyphs);
        } break;

        case(DIRT_SPLIT_VERTICAL):
        {
          Structures::Container leftContainer;
          leftContainer.pos[0] = splitBox->container.pos[0];
          leftContainer.pos[1] = splitBox->container.pos[1];
          leftContainer.width = splitBox->container.width/2;
          leftContainer.height = splitBox->container.height;

          Structures::Container rightContainer;
          rightContainer.pos[0] = splitBox->container.pos[0] + splitBox->container.width/2;
          rightContainer.pos[1] = splitBox->container.pos[1];
          rightContainer.width = splitBox->container.width/2;
          rightContainer.height = splitBox->container.height;

          splitBox->top = createSplitBox(leftContainer, childGlyphs);
          splitBox->bottom = createSplitBox(rightContainer, childGlyphs);
        } break;
      }
    }
  }
}
