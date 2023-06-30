#include <windows.h>
#include <stdio.h>

#define BUFSIZE MAX_PATH

int main(int argc, char **argv)
{
  if(argc != 3)
  {
    printf("ERROR: Incorrect number of arguments\n\n");
    printf("Description: \n\tMoves a directory and its contents\n\n");
    printf("Usage:\n\t%s <source_dir> <target_dir>\n\n", argv[0]);
    return 1;
  }

  if(!MoveFileEx(argv[1], argv[2], MOVEFILE_WRITE_THROUGH))
  {
    printf("MoveFileEx failed with error %d\n", (int)GetLastError());
    return 1;
  }
  printf("%s has been moved to %s\n", argv[1], argv[2]);

  return 0;
}
