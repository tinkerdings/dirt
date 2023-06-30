#include <windows.h>
#include <stdio.h>

#define BUFSIZE MAX_PATH

int main(int argc, char **argv)
{
  DWORD ret;
  char dirName[BUFSIZE];

  /* if(argc != 2) */
  /* { */
  /*   printf("Usage: %s <dir>\n", argv[0]); */
  /*   return 1; */
  /* } */

  printf("Getting current directory....\n");

  ret = GetCurrentDirectory(BUFSIZE, dirName);

  if(!ret)
  {
    printf("GetCurrentDirectory failed (%d)\n", GetLastError());
    return 1;
  }

  if(ret > BUFSIZE)
  {
    printf("Buffer too small. Need %d characters for current dir path name\n", ret);
    return 1;
  }

  if(!dirName)
  {
    printf("Failed to get current directory name (%d)\n", GetLastError());
    return 1;
  }
  printf("Current Directory: %s\n", dirName);

  return 0;
}
