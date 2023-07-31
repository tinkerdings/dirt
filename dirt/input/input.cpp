#include <windows.h>
#include <DbgHelp.h>
#include <stdio.h>

#include <dirt/input/input.h>
#include <dirt/context/context.h>
#include <dirt/screen/screen.h>
#include <dirt/error/errorCode.h>
#include <dirt/entry/entry.h>
#include <dirt/rendering/rendering.h>

namespace Dirt
{
  namespace Input
  {
    void handleInput(Dirt::Context *context, ScreenData &screen, HANDLE stdinHandle)
    {
      INPUT_RECORD inputBuffer[INPUTBUF_SIZE] = {};
      DWORD nRecordsRead = 0;
      if(!ReadConsoleInput(
        stdinHandle, inputBuffer, 3, &nRecordsRead))
      {
        printf("ReadConsoleInput failed (%lu)\n", GetLastError());
        return;
      }

      for(int inputRecIndex = 0; inputRecIndex < nRecordsRead; inputRecIndex++)
      {
        WIN32_FIND_DATA &activeEntry = screen.active->entries[screen.active->cursorIndex.actualIndex];
        switch(inputBuffer[inputRecIndex].EventType)
        {
          case(KEY_EVENT):
          {
            if(!inputBuffer[inputRecIndex].Event.KeyEvent.bKeyDown)
            {
              context->input.prevKeyCode = -1;
              break;
            }
            WORD vKeyCode = inputBuffer[inputRecIndex].Event.KeyEvent.wVirtualKeyCode;
            switch(vKeyCode)
            {
              case(VK_Q):
              {
                context->quit = true;
                return;
              } break;
              case(VK_DOWN):
              case(VK_J):
              {
                incrementScreenCursorIndex(context, screen);
              } break;
              case(VK_UP):
              case(VK_K):
              {
                decrementScreenCursorIndex(context, screen);
              } break;
              case(VK_TAB):
              {
                if(screen.active == &screen.leftView)
                {
                  screen.active = &screen.rightView;
                }
                else 
                {
                  screen.active = &screen.leftView;
                }
                if(!SetCurrentDirectory(screen.active->path))
                {
                  printf("Failed to set current directory (%lu)\n", GetLastError());
                }
              } break;
              case(VK_RIGHT):
              case(VK_L):
              {
                if(activeEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                  setViewPath(context, *screen.active, activeEntry.cFileName);
                }
              } break;
              case(VK_RETURN):
              {
                char fullPath[MAX_PATH] = {};
                if(!Entry::getFullPath(fullPath, activeEntry.cFileName, MAX_PATH))
                {
                  printf("fullPath failed (%lu)\n", GetLastError());
                  break;
                }
                DWORD binaryType = -1;
                // TODO: Error handling for file opening
                if(GetBinaryTypeA(fullPath, &binaryType))
                {
                  if(binaryType == SCS_32BIT_BINARY || binaryType == SCS_64BIT_BINARY)
                  {
                    HANDLE exeHandle = CreateFileA(
                      fullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                    if(exeHandle == INVALID_HANDLE_VALUE)
                    {
                      printf("CreateFileA failed (%lu)\n", GetLastError());
                      break;
                    }
                    HANDLE exeMapping = CreateFileMappingA(
                      exeHandle,
                      0,
                      PAGE_READONLY, 0, 0, 0);
                    if(!exeMapping || exeMapping == INVALID_HANDLE_VALUE)
                    {
                      printf("CreateFileMappingA failed (%lu)\n", GetLastError());
                      break;
                    }
                    void* exe = MapViewOfFile(
                      exeMapping, FILE_MAP_READ, 0, 0, 0);
                    PIMAGE_NT_HEADERS ntHeaders = ImageNtHeader(exe);
                    switch(ntHeaders->OptionalHeader.Subsystem)
                    {
                      case(IMAGE_SUBSYSTEM_WINDOWS_CUI):
                      {
                        ShellExecuteA(0, 0, fullPath, 0, 0, SW_SHOW);
                      } break;
                      case(IMAGE_SUBSYSTEM_WINDOWS_GUI):
                      {
                        PROCESS_INFORMATION exeInfo;
                        STARTUPINFOA startupInfo = {
                          sizeof(STARTUPINFOA),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          STARTF_USESHOWWINDOW, SW_SHOW,
                          0, 0, 0, 0, 0
                        };
                        CreateProcessA(fullPath, 
                          0, 0, 0, false,
                          NORMAL_PRIORITY_CLASS,
                          0, 0,
                          &startupInfo, &exeInfo);
                      } break;
                    }
                  }
                }
                else 
                {
                  if(ShellExecuteA(0, 0, fullPath, 0, 0, SW_SHOW) <= (HINSTANCE)32)
                  {
                    printf("ShellExecuteA failed (%lu)\n", GetLastError());
                    break;
                  }
                }
              } break;
              case(VK_X):
              {
                char fullPath[MAX_PATH] = {};
                if(!Entry::getFullPath(fullPath, screen.active->path, MAX_PATH))
                {
                  printf("fullPath failed (%lu)\n", GetLastError());
                  break;
                }
                if(ShellExecuteA(0, 0, "cmd.exe", 0, 0, SW_SHOW) <= (HINSTANCE)32)
                {
                  printf("ShellExecuteA failed (%lu)\n", GetLastError());
                  break;
                }
              } break;
              case(VK_E):
              {
                char fullPath[MAX_PATH] = {};
                if(!Entry::getFullPath(fullPath, screen.active->path, MAX_PATH))
                {
                  printf("fullPath failed (%lu)\n", GetLastError());
                  break;
                }
                PROCESS_INFORMATION exeInfo;
                STARTUPINFOA startupInfo = {
                  sizeof(STARTUPINFOA),
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  STARTF_USESHOWWINDOW, SW_SHOW,
                  0, 0, 0, 0, 0
                };
                char explorerPath[MAX_PATH] = {};
                FindExecutableA("explorer.exe", 0, explorerPath);
                char arg[MAX_PATH] = {};
                strcat(arg, " /e,/root,");
                strcat(arg, fullPath);
                CreateProcessA(explorerPath,
                  arg,
                  0, 0, false,
                  NORMAL_PRIORITY_CLASS,
                  0, 0,
                  &startupInfo, &exeInfo);
              } break;
              case(VK_BACK):
              case(VK_LEFT):
              case(VK_H):
              {
                /* if(!isRootDir(screen.active.path)) */ // TODO Implement this
                /* { */
                setViewPath(context, *screen.active, "..");
                /* } */
              } break;
              case(VK_ESCAPE):
              {
                Dirt::Entry::clearAllSelection(context);
              } break;
              case(VK_SPACE):
              {
                /* if(inputBuffer[i].Event.KeyEvent.dwControlKeyState == SHIFT_PRESSED) */
                /* { */
                /* } */
                if(inputNoKeyRepeat(context, inputBuffer, inputRecIndex, INPUTBUF_SIZE))
                {
                  char fullPath[MAX_PATH] = {};
                  if(!Entry::getFullPath(fullPath, activeEntry.cFileName, MAX_PATH))
                  {
                    printf("getFullPath failed (%lu)\n", GetLastError());
                    break;
                  }
                  if(hashmapContains(context->selection, fullPath, MAX_PATH, 0, 0))
                  {
                    Dirt::Entry::removeEntryFromSelection(context, fullPath);
                  }
                  else 
                  {
                    if(!Entry::addEntryToSelection(context, fullPath))
                    {
                      printf("addEntryToSelection failed\n");
                      break;
                    }
                  }
                }
              } break;
              case(VK_M):
              {
                Dirt::Entry::moveSelection(context);
                Dirt::Entry::clearAllSelection(context);
              } break;
              case(VK_D):
              {
                Dirt::Entry::deleteSelection(context);
                Dirt::Entry::clearAllSelection(context);
              } break;
              case(VK_R):
              {
                Rendering::refresh(context, screen);
              } break;
              case(VK_1):
              case(VK_2):
              case(VK_3):
              case(VK_4):
              case(VK_5):
              case(VK_6):
              case(VK_7):
              case(VK_8):
              case(VK_9):
              {
                Screen::setCurrentScreen(context, vKeyCode-VK_1);
              } break;
            }

            context->input.prevKeyCode = 
                inputBuffer[inputRecIndex].Event.KeyEvent.wVirtualKeyCode;
          } break;
        }
      }
    }

    bool inputNoKeyRepeat(Dirt::Context *context, INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size)
    {
      WORD keyCode = inputBuffer[index].Event.KeyEvent.wVirtualKeyCode;

      if(keyCode == context->input.prevKeyCode)
      {
        return false;
      }

      return true;
    }
  } // namespace Input
} // namespace Dirt

