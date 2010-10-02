RECORDER_PROGRAM=cvlc

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

# You must configure the right password; I put the default one
ICECAST_PASSWORD=hackme
ICECAST_ADDRESS=127.0.0.1
ICECAST_PORT=8000
ICECAST_MOUNTPOINT=ardesia.ogg

SCRIPT_FOLDER=`dirname "$0"`
RECORDER_PID_FILE=/tmp/recorder.pid


if [ "$1" = "start" ]
then
  #This start the recording on file
  echo Start the screencast running $RECORDER_PROGRAM
  if [ "$ICECAST" = "TRUE" ]
  then
    RECORDER_PROGRAM_OPTIONS="-vvv screen:// :input-slave=alsa://pulse --screen-fps=12 --sout "#transcode{venc=theora,vcodec=theo,vb=800,acodec=vorb,ab=128,channels=2,samplerate=44100,scale=0.75,fps=12}:duplicate{dst=std{access=shout,mux=ogg,dst=source:$ICECAST_PASSWORD@$ICECAST_ADDRESS:$ICECAST_PORT/$ICECAST_MOUNTPOINT},dst=std{access=file,mux=ogg,dst=$2}}"" 
  else
    RECORDER_PROGRAM_OPTIONS="-vvv screen:// :input-slave=alsa://pulse --screen-fps=12 --sout "#transcode{venc=theora,vcodec=theo,vb=800,acodec=vorb,ab=128,channels=2,samplerate=44100,scale=0.75,fps=12}:duplicate{dst=std{access=file,mux=ogg,dst=$2}}"" 
  fi
  echo With arguments $RECORDER_PROGRAM_OPTIONS
  $RECORDER_PROGRAM $RECORDER_PROGRAM_OPTIONS &
  RECORDER_PID=$!
  echo $RECORDER_PID >> $RECORDER_PID_FILE
fi

if [ "$1" = "stop" ]
then
  RECORDER_PID=$(cat $RECORDER_PID_FILE)
  echo Stop the screencast killing $RECORDER_PROGRAM 
  kill -2 $RECORDER_PID 
  rm $RECORDER_PID_FILE
fi


