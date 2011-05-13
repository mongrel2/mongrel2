#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/mp3stream
# WARNING: on some systems the nohup doesn't work, like OSX
# try running it without
nohup python -u handler.py 2>&1 > mp3stream.log &
echo $! > $DEPLOY/profiles/mp3stream/mp3stream.pid
