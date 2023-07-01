#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <winerror.h>
#include <winnt.h>

int main(int argc, char **argv)
{
  WIN32_FIND_DATA fileInfo;
  LARGE_INTEGER fileSize;
  HANDLE entry = INVALID_HANDLE_VALUE;
  DWORD error = 0;
  char dirName[MAX_PATH] = {0};

  if(argc != 2)
  {
    printf("Usage: %s <directory name>\n", argv[0]);
    return 1;
  }
  if(strlen(argv[1]) > (MAX_PATH - 3))
  {
    printf("Directory path is too long.\n");
    return 1;
  }

  printf("Target directory is %s\n", argv[1]);
  strcpy(dirName, argv[1]);
  strcat(dirName, "\\*");

  entry = FindFirstFile(dirName, &fileInfo);
  if(entry == INVALID_HANDLE_VALUE)
  {
    printf("Failed to find file (%d)\n", GetLastError());
    return 1;
  }

  do
  {
    if(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      printf("  %s   <DIR>\n", fileInfo.cFileName);
    }
    else 
    {
      fileSize.LowPart = fileInfo.nFileSizeLow;
      fileSize.HighPart = fileInfo.nFileSizeHigh;
      printf("  %s   %lld bytes\n", fileInfo.cFileName, fileSize.QuadPart);
    }
  }
  while(FindNextFile(entry, &fileInfo));

  error = GetLastError();
  if(error != ERROR_NO_MORE_FILES)
  {
    printf("error: %d\n", error);
    return 1;
  }

  FindClose(entry);

  return error;
}
