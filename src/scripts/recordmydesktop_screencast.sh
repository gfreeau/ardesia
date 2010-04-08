ICECAST="FALSE";
#These are the right variables 
#supposing that the icecast server
#is running in the local machine 
#with the default conf file
LOCATION="localhost"
PORT="8000"
PASSWORD="hackme"
MOUNTPOINT="/ardesia.ogv"
# Uncomment this to use the live screencast on icecast
# To start the live stream successfully you must configure
# the previous variables as you desire to point to the right
# running icecast server
# Client side you must have the oggfwd installed
#ICECAST="TRUE";

if [ "$2" = "start" ]
then
  mkfifo $1
  #This start the recording on file
  echo Start the screencast running recordmydesktop 
  recordmydesktop --quick-subsampling --on-the-fly-encoding -v_quality 10 -v_bitrate 50000 -s_quality 1 --fps 10 --freq 48000 --buffer-size 16384 -device plughw:0,0 --overwrite -o $1 &
  RECORDPID=$!
  echo $RECORDPID >> /tmp/recordmydesktop.pid
  if [ "$ICECAST" = "TRUE" ]
  then
    echo Sending the streaming to icecast server $LOCATION 
    cat $1 | oggfwd $LOCATION $PORT $PASSWORD $MOUNTPOINT & 
    OGGFWDPID=$!
    echo $OGGFWDPID >> /tmp/oggfwd.pid
  fi
fi

if [ "$2" = "stop" ]
then
  RECORDPID=$(cat /tmp/recordmydesktop.pid)
  echo Stop the screencast killing recordmydesktop 
  kill -9 $RECORDPID 
  rm /tmp/recordmydesktop.pid
  if [ "$ICECAST" = "TRUE" ]
  then
    OGGFWDPID=$(cat  /tmp/oggfwd.pid)
    echo Stopping the streaming to icecast server $LOCATION 
    kill -9 $OGGFWDPID
    rm /tmp/oggfwd.pid
  fi
fi


