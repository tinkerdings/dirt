#include <windows.h>
#include <stdio.h>

#define BUFSIZE MAX_PATH

int main(int argc, char **argv)
{
  DWORD ret;
  char dirName[BUFSIZE];

  if(argc != 2)
  {
    printf("Usage: %s <dir>\n", argv[0]);
    return 1;
  }

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

  if(!SetCurrentDirectory(argv[1]))
  {
    printf("SetCurrentDirectory failed (%d)\n", GetLastError());
    return 1;
  }
  printf("Set current directory to %s\n", argv[1]);

  if(!SetCurrentDirectory(dirName))
  {
    printf("SetCurrentDirectory failed (%d)\n", GetLastError());
    return 1;
  }
  printf("Restored previous directory (%s)\n", dirName);

  return 0;
}
