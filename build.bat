@echo off

mkdir .\build
pushd .\build

cl -FC -Zi ..\dirt.cpp

popd
