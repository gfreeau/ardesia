@echo off
rem Uncomment this to use the live screencast on icecast
rem To start the live stream successfully you must configure
rem the configuration file ezstream_stdin_vorbis.xml
rem as you desire to point to the right running icecast server
rem The configuration file is setted to work properly
rem if it is used a localhost icecast server with the default 
rem configuration
rem Client side you must have the ezstream installed

rem set ICECAST=FALSE
set ICECAST=TRUE

rem You must configure the right password; I put the default one
set ICECAST_PASSWORD=hackme
set ICECAST_ADDRESS=127.0.0.1
set ICECAST_PORT=8000
set ICECAST_MOUNTPOINT=ardesia.ogg

set RECORDER_PROGRAM=vlc.exe
set RECORDER_PROGRAM_OPTIONS=-vvv -I dummy screen:// --screen-fps=12 --sout "#transcode{venc=theora,quality:10,scale=0.75,fps=12}:duplicate{dst=std{access=file,mux=ogg,dst=%2}}"
set RECORDER_AND_FORWARD_PROGRAM_OPTIONS=-vvv -I dummy screen:// --screen-fps=12 --sout "#transcode{venc=theora,quality:10,scale=0.75,fps=12}:duplicate{dst=std{access=shout,mux=ogg,dst=source:%ICECAST_PASSWORD%@%ICECAST_ADDRESS%:%ICECAST_PORT%/%ICECAST_MOUNTPOINT%},dst=std{access=file,mux=ogg,dst=%2}}"

if ""%1"" == ""start"" goto start

if ""%1"" == ""stop"" goto stop

:start
rem This start the recording on file
if "%ICECAST%" == "TRUE" goto icecast_start
rem if not icecast then only record the screencast
echo Start the screencast running %RECORDER_PROGRAM%
echo With arguments %RECORDER_PROGRAM_OPTIONS%
rem exec recorder
start /B cmd /c "%RECORDER_PROGRAM% %RECORDER_PROGRAM_OPTIONS%"
goto end

:icecast_start
echo Start the screencast running %RECORDER_PROGRAM%
echo With arguments %RECORDER_AND_FORWARD_PROGRAM_OPTIONS%
rem exec recorder
start /B cmd /c "%RECORDER_PROGRAM% %RECORDER_AND_FORWARD_PROGRAM_OPTIONS%"
goto end

:stop
set RECORDER_PID=cat %RECORDER_PID_FILE%
echo Stop the screencast killing %RECORDER_PROGRAM% 
TASKKILL /F /IM %RECORDER_PROGRAM%
goto end

:end
