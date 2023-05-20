cd /d C:\Emscripten\emsdk-3.1.23 
call emsdk_env.bat

cd /d D:\JO_Programowanie\Projekty\CompilerProject
if not exist wasm\debug mkdir wasm\debug
del wasm\debug /Q

set disable-warnings=-Wno-switch -Wno-unused-value -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-sign-compare -Wno-typedef-redefinition -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers -Wno-limited-postlink-optimizations

set warnings-options=-Wall -Wextra %disable-warnings%

set general-options=-D_DEFAULT_SOURCE -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=48MB -sTOTAL_STACK=2MB -sNO_EXIT_RUNTIME=1 -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS="['_main', '_compile_input']" -sEXPORTED_RUNTIME_METHODS="['ccall', 'callMain']"

set debug-options=-DDEBUG_BUILD -sASSERTIONS=2 -o wasm/debug/index.html -O0 -g3 -gsource-map --source-map-base http://localhost:6931/ --emrun --memoryprofiler --shell-file emscripten_debug_shell.html

call emcc -std=c99 %warnings-options% %general-options% %debug-options% main.c 

pause

call emrun --browser "C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe" wasm/debug/index.html