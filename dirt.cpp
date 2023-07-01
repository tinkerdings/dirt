#include <windows.h>
#include <stdio.h>

VOID ErrorExit(LPCSTR);
VOID KeyEventProc(KEY_EVENT_RECORD);
VOID MouseEventProc(MOUSE_EVENT_RECORD);
VOID ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD);

HANDLE stdinHandle = INVALID_HANDLE_VALUE;
DWORD oldModeSave;

int main(int argc, char **argv)
{
  DWORD nRecordsRead, mode;
  INPUT_RECORD buffer[128];

  stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
  if(stdinHandle == INVALID_HANDLE_VALUE)
  {
    ErrorExit("GetStdHandle");
  }

  if(!GetConsoleMode(stdinHandle, &oldModeSave))
  {
    ErrorExit("GetConsoleMode");
  }

  mode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
  if(!SetConsoleMode(stdinHandle, mode))
  {
    ErrorExit("SetConsoleMode");
  }

  for(int i = 0; i < 100; i++)
  {
    if(!ReadConsoleInput(
      stdinHandle,
      buffer, 128,
      &nRecordsRead))
    {
      ErrorExit("ReadConsoleInput");
    }

    for(int j = 0; j < nRecordsRead; j++)
    {
      switch(buffer[j].EventType)
      {
        case(KEY_EVENT):
        {
          KeyEventProc(buffer[j].Event.KeyEvent);
        } break;
        case(MOUSE_EVENT):
        {
          printf("MOUSE EVENT!");
          MouseEventProc(buffer[j].Event.MouseEvent);
        } break;
        case(WINDOW_BUFFER_SIZE_EVENT):
        {
          ResizeEventProc(buffer[j].Event.WindowBufferSizeEvent);
        } break;
        case(FOCUS_EVENT):
        case(MENU_EVENT):
        {
        } break;
        default:
        {
        } break;
      }
    }
  }

  SetConsoleMode(stdinHandle, oldModeSave);

  return 0;
}

VOID ErrorExit(LPCSTR errorMessage)
{
  fprintf(stderr, "error %s\n", errorMessage);
  SetConsoleMode(stdinHandle, oldModeSave);
  ExitProcess(0);
}

VOID KeyEventProc(KEY_EVENT_RECORD rec)
{
  printf("Key event: ");
  if(rec.bKeyDown)
  {
    printf("key pressed\n");
  }
  else
  {
    printf("key released\n");
  }
}

VOID MouseEventProc(MOUSE_EVENT_RECORD rec)
{
#ifndef MOUSE_HWHEELED
#define MOUSE_HWHEELED 0x0008
#endif
  printf("Mouse event: ");
  
  switch(rec.dwEventFlags)
  {
    case(0):
    {
      if(rec.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
      {
        printf("left button press \n");
      }
      else if(rec.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
      {
        printf("right button press \n");
      }
      else 
      {
        printf("button press\n");
      }
    } break;
    case(DOUBLE_CLICK):
    {
      printf("double click\n");
    } break;
    case(MOUSE_HWHEELED):
    {
      printf("horizontal mouse wheel\n");
    } break;
    case(MOUSE_MOVED):
    {
      printf("mouse moved\n");
    } break;
    case(MOUSE_WHEELED):
    {
      printf("vertical mouse wheel\n");
    } break;
    default:
    {
      printf("unknown\n");
    } break;
  }
}

VOID ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD rec)
{
  printf("Resize event\n");
  printf("Console screen buffer is %d columns by %d rows. \n",
    rec.dwSize.X, rec.dwSize.Y);
}
