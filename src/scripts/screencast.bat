@echo off
rem Uncomment this to use the live screencast on icecast
rem To start the live stream successfully you must configure
rem the configuration file ezstream_stdin_vorbis.xml
rem as you desire to point to the right running icecast server
rem The configuration file is setted to work properly
rem if it is used a localhost icecast server with the default 
rem configuration
rem Client side you must have the ezstream installed

set ICECAST="FALSE";
rem set ICECAST="TRUE";

rem This is the location of the vlc executable
set RECORDER_LOCATION="C:\Program Files (x86)\VideoLAN\VLC"

set RECORDER_PROGRAM=vlc.exe
set RECORDER_PROGRAM_OPTIONS=-vvv -I dummy screen:// --screen-fps=12 --sout "#transcode{venc=theora,quality:10,scale=0.75,fps=12}:duplicate{dst=std{access=file,mux=ogg,dst=%2}}}"
set STREAMER_COMMAND=ezstream.exe
set STREAMER_OPTIONS=-v -c ezstream_stdin_ardesia.xml 

if ""%1"" == ""start"" goto start

if ""%1"" == ""stop"" goto stop

:start
rem This start the recording on file
echo Start the screencast running %RECORDER_PROGRAM%
echo With arguments %RECORDER_PROGRAM_OPTIONS%
rem exec recorder
start /B cmd /c "%RECORDER_LOCATION%\%RECORDER_PROGRAM% %RECORDER_PROGRAM_OPTIONS%"
if %ICECAST% == "TRUE" goto icecast_start
goto end

:icecast_start
rem wait some time
ping -n 20 -w 1000 0.0.0.1 >NUL 2>&1
rem start the stream to icecast with ezstream
echo Forwarding the streaming %2 to icecast server 
start /B cmd /c "type %2 | %STREAMER_COMMAND% %STREAMER_OPTIONS%"
goto end

:stop
set RECORDER_PID=cat %RECORDER_PID_FILE%
echo Stop the screencast killing %RECORDER_PROGRAM% 
TASKKILL /F /IM %RECORDER_PROGRAM%
if %ICECAST% == ""TRUE"" goto icecast_stop
goto end

:icecast_stop
echo Stopping the streaming to icecast server
rem stop the stream to icecast killing ezstream
TASKKILL /F /IM %STREAMER_COMMAND%
goto end

:end
