cd /d C:\Emscripten\emsdk-3.1.23 
call emsdk_env.bat

cd /d D:\JO_Programowanie\Projekty\CompilerProject
if not exist wasm\debug mkdir wasm\debug

set disable-warnings=-Wno-switch -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-typedef-redefinition -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers

set debug-flags=-DDEBUG_BUILD -sASSERTIONS=1 

set flags=-D_DEFAULT_SOURCE -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=128MB -sTOTAL_STACK=32MB -sNO_EXIT_RUNTIME=1 -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS="['_main']" -sEXPORTED_RUNTIME_METHODS="['ccall', 'callMain']" --preload-file test

set output-options=-o wasm/debug/index.html -O0 -g3 -gsource-map --source-map-base http://localhost:6931/ --emrun --memoryprofiler --shell-file emscripten_shell.html

call emcc -std=c99 -Wall -Wextra %flags% %debug-flags% %disable-warnings% main.c %output-options%

pause

call emrun --browser "C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe" wasm/debug/index.html