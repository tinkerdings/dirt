@echo off

mkdir ..\build
pushd ..\build

cl -FC -Zi ..\dirt\dirt.cpp ..\dirt\structures\hashmap.cpp ..\dirt\memory\memory.cpp -I..\ -link shell32.lib ole32.lib dbghelp.lib Shlwapi.lib

popd
