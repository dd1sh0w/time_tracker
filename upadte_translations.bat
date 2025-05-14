@echo off
REM Update Translation Files Script

echo Updating translation files...

REM Set path to Qt tools - adjust according to your Qt installation
set QT_DIR=c:\Qt\6.8.2\mingw_64
set PATH=%QT_DIR%\bin;%PATH%

REM Extract strings from source code
lupdate -recursive ./src -ts ./src/resources/translations/time_tracker_en_US.ts ./src/resources/translations/time_tracker_ru_RU.ts

echo.
echo Translation files updated!
echo Now you can edit the translation files using Qt Linguist.
echo After editing, run CMake to build the project which will convert TS files to QM files.
echo.

pause