#include <windows.h>
#include <math.h>
#include <stdio.h>

#define BUFSIZE MAX_PATH

int main(int argc, char **argv)
{
  HANDLE stdoutHandle, backBuffer, frontBuffer;
  SMALL_RECT readRect, writeRect;
  /* CHAR_INFO buffer[400]; */
  CHAR_INFO emptyBuffer[8000];
  for(int i = 0; i < 8000; i++)
  {
    emptyBuffer[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
  }
  CHAR_INFO buffer[] = {
    {'H', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'E', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'L', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'L', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'O', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'T', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'H', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'E', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'R', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {'E', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE}
  };

  COORD bufSize, bufCoords;
  COORD emptyBufSize, emptyBufCoords;
  BOOL success;

  stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

  backBuffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL);
  frontBuffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL);

  if(stdoutHandle == INVALID_HANDLE_VALUE ||
      backBuffer == INVALID_HANDLE_VALUE ||
      frontBuffer == INVALID_HANDLE_VALUE)
  {
    printf("CreateConsoleScreenBuffer failed (%d)\n", (int)GetLastError());
    return 1;
  }

  /* if(!SetConsoleActiveScreenBuffer(newScreenBufferHandleA)) */
  /* { */
  /*   printf("SetConsoleActiveScreenBuffer failed (%d)\n", (int)GetLastError()); */
  /*   return 1; */
  /* } */

  readRect.Top = 0;
  readRect.Left = 0;
  readRect.Bottom = 20;
  readRect.Right = 20;

  bufSize.Y = 1;
  bufSize.X = 11;
  bufCoords.X = 0;
  bufCoords.Y = 0;
  emptyBufSize.Y = 80;
  emptyBufSize.X = 100;
  emptyBufCoords.X = 0;
  emptyBufCoords.Y = 0;

  /* success = ReadConsoleOutput(stdoutHandle, buffer, bufSize, bufCoords, &readRect); */
  /* if(!success) */
  /* { */
  /*   printf("ReadConsoleOutput failed (%d)\n", (int)GetLastError()); */
  /*   return 1; */
  /* } */

#define PI 3.14159

  float i = 0.0;
  float radius = 20.0;
  SMALL_RECT clearRect;
  clearRect.Top = 10;
  clearRect.Left = 62;
  clearRect.Bottom = 49;
  clearRect.Right = 150;

  while(1)
  {
    writeRect.Top = 30+sin(i)*radius;
    writeRect.Left = 100+cos(i)*radius*1.9;
    writeRect.Bottom = writeRect.Top + 20;
    writeRect.Right = writeRect.Left + 20;
    i += 0.002;
    if(i > 2*PI)
    {
      i = 0.0;
    }

    success = WriteConsoleOutput(backBuffer, emptyBuffer, emptyBufSize, emptyBufCoords, &clearRect);
    if(!success)
    {
      printf("WriteConsoleOutput failed (%d)\n", (int)GetLastError());
      return 1;
    }
    success = WriteConsoleOutput(backBuffer, buffer, bufSize, bufCoords, &writeRect);
    if(!success)
    {
      printf("WriteConsoleOutput failed (%d)\n", (int)GetLastError());
      return 1;
    }
    HANDLE swap = frontBuffer;
    frontBuffer = backBuffer;
    backBuffer = swap;

    if(!SetConsoleActiveScreenBuffer(frontBuffer))
    {
      printf("SetConsoleActiveScreenBuffer failed (%d)\n", (int)GetLastError());
      return 1;
    }
  }

  if(!SetConsoleActiveScreenBuffer(stdoutHandle))
  {
    printf("SetConsoleActiveScreenBuffer failed (%d)\n", (int)GetLastError());
    return 1;
  }

  return 0;
}
