#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/chat
python -u chat.py 2>&1 > chat.log &
echo $! > $DEPLOY/profiles/chat/chat.pid
