cd /d C:\Emscripten\emsdk-3.1.23 
call emsdk_env.bat

cd /d D:\JO_Programowanie\Projekty\CompilerProject
if not exist wasm\release mkdir wasm\release

set disable-warnings=-Wno-switch -Wno-unused-value -Wno-typedef-redefinition -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers

call emcc -std=c99 -D_DEFAULT_SOURCE %disable-warnings% main.c -o wasm/release/index.html --shell-file emscripten_shell.html

del "wasm\release\lang.zip"
"C:\Program Files\7-Zip\7z.exe" a -tzip "wasm\release\lang.zip" %CD%\wasm\release\*