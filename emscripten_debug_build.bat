cd /d C:\Emscripten\emsdk-3.1.23 
call emsdk_env.bat

cd /d D:\JO_Programowanie\Projekty\CompilerProject
if not exist wasm\debug mkdir wasm\debug

set disable-warnings=-Wno-switch -Wno-unused-value -Wno-typedef-redefinition -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers

call emcc -std=c99 -DDEBUG_BUILD -D_DEFAULT_SOURCE %disable-warnings% main.c -o wasm/debug/index.html -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=128MB -sTOTAL_STACK=32MB -g3 -gsource-map --source-map-base http://localhost:6931/ --emrun --memoryprofiler

pause

call emrun --browser "C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe" wasm/debug/index.html