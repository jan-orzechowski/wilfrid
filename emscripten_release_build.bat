cd /d C:\Emscripten\emsdk-3.1.23 
call emsdk_env.bat

cd /d D:\JO_Programowanie\Projekty\CompilerProject
if not exist wasm\release mkdir wasm\release
del wasm\release /Q

set disable-warnings=-Wno-switch -Wno-unused-value -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-sign-compare -Wno-typedef-redefinition -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers

set warnings-options=-Wall -Wextra %disable-warnings%

set general-options=-D_DEFAULT_SOURCE -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=48MB -sTOTAL_STACK=2MB -sNO_EXIT_RUNTIME=1 -sINVOKE_RUN=0 -sEXPORTED_FUNCTIONS="['_main', '_compile_input']" -sEXPORTED_RUNTIME_METHODS="['ccall', 'callMain']" --embed-file include

set release-options=-sSAFE_HEAP=0 -sASSERTIONS=0 -o wasm/release/nous.js -O3 --pre-js emscripten_module_functions.js

call emcc -std=c99 %warnings-options% %general-options% %release-options% main.c 

pause

REM a script to concatenate all examples into a JS file used on the website

@echo off

set output_js=wasm\release\examples.js
echo. > %output_js%

echo let all_examples = {}; >> %output_js%
for %%I in ("examples\*") do (
	echo let example_%%~nI = >> %output_js%
	echo ` >> %output_js%
	type "%%I" >> %output_js%
	echo `; >> %output_js%
	echo all_examples[^"example_%%~nI^"] = example_%%~nI; >> %output_js%
)

pause
