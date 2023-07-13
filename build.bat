@echo off

mkdir .\build
pushd .\build

cl -FC -Zi ..\dirt.cpp -link shell32.lib ole32.lib dbghelp.lib Shlwapi.lib

popd
