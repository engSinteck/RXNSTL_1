@echo off
cls
@echo on
cd ..\..\Sinteck\html\
del fsdata_custom.c
makefsdata.exe -11
ren fsdata.c fsdata_custom.c
copy fsdata_custom.c ..\..\Middlewares\Third_Party\LwIP\src\apps\http\.
