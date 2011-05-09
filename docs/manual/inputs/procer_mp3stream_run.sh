#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/mp3stream
nohup python -u handler.py 2>&1 > mp3stream.log &
echo $! > $DEPLOY/profiles/mp3stream/mp3stream.pid
