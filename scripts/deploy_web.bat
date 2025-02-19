set BUILD_DIR=..\build\WebAssembly_Qt_6_8_2_single_threaded-Release
set DEPLOY_DIR=..\deployed\web

rmdir /s /q %DEPLOY_DIR%
mkdir %BUILD_DIR%
mkdir %DEPLOY_DIR%

:: Move output files to deploy folder
echo Moving build output to %DEPLOY_DIR%...
copy %BUILD_DIR%\*.html %DEPLOY_DIR%\index.html
copy %BUILD_DIR%\*.js %DEPLOY_DIR%\
copy %BUILD_DIR%\*.wasm %DEPLOY_DIR%\
copy %BUILD_DIR%\*.svg %DEPLOY_DIR%\

echo Build complete. Files are in %DEPLOY_DIR%