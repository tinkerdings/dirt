@echo off

pushd ..\build

pushd .\test
del /Q test*
call gentf 60
popd

set ARG=%1

.\dirt.exe %1%

 popd
