@echo off

mkdir ..\build
pushd ..\build

set sources=..\dirt\dirt.cpp ..\dirt\structures\hashmap.cpp ..\dirt\memory\memory.cpp ..\dirt\error\errorCode.cpp ..\dirt\input\input.cpp ..\dirt\screen\screen.cpp ..\dirt\entry\entry.cpp

cl -FC -Zi  %sources% -I..\ -link shell32.lib ole32.lib dbghelp.lib Shlwapi.lib

popd
