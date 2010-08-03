RECORDER_PROGRAM=recordmydesktop
RECORDER_PROGRAM_OPTIONS="--quick-subsampling --on-the-fly-encoding --v_quality 1 --s_quality 1 --fps 10 --freq 22050 --buffer-size 16384 --device plughw:0,0 --overwrite -o $2"
 
#RECORDER_PROGRAM=cvlc
#RECORDER_PROGRAM_OPTIONS="screen:// --screen-fps=12 --sout "#transcode{venc=theora,quality:10,scale=0.75,fps=12}:duplicate{dst=std{access=file,mux=ogg,dst=$2}}}"" 


# Uncomment this to use the live screencast on icecast
# To start the live stream successfully you must configure
# the configuration file ezstream_stdin_vorbis.xml
# as you desire to point to the right running icecast server
# The configuration file is setted to work properly
# if it is used a localhost icecast server with the default 
# configuration
# Client side you must have the ezstream installed
ICECAST="FALSE"
#ICECAST="TRUE"

STREAMER_COMMAND=ezstream
STREAMER_OPTIONS="-vv -c $SCRIPT_FOLDER/ezstream_stdin_ardesia.xml"

SCRIPT_FOLDER=`dirname "$0"`
RECORDER_PID_FILE=/tmp/recorder.pid
STREAMER_PID_FILE=/tmp/ezstream.pid


if [ "$1" = "start" ]
then
  #This start the recording on file
  echo Start the screencast running $RECORDER_PROGRAM
  echo With arguments $RECORDER_PROGRAM_OPTIONS
  $RECORDER_PROGRAM $RECORDER_PROGRAM_OPTIONS &
  echo Running $COMMAND
  RECORDER_PID=$!
  echo $RECORDER_PID >> $RECORDER_PID_FILE
  if [ "$ICECAST" = "TRUE" ]
  then
    sleep 20
    echo Sending $2 stream to icecast server $LOCATION 
    cat $2 | $STREAMER_COMMAND $STREAMER_OPTIONS &
    EZSTREAM_PID=$!
    echo $EZSTREAM_PID >> $STREAMER_PID_FILE
  fi
fi

if [ "$1" = "stop" ]
then
  RECORDER_PID=$(cat $RECORDER_PID_FILE)
  echo Stop the screencast killing $RECORDER_PROGRAM 
  kill -2 $RECORDER_PID 
  rm $RECORDER_PID_FILE
  if [ "$ICECAST" = "TRUE" ]
  then
    EZSTREAM_PID=$(cat  $STREAMER_PID_FILE)
    echo Stopping the streaming to icecast server $LOCATION 
    kill -15 $EZSTREAM_PID
    rm /tmp/ezstream.pid
  fi
fi


