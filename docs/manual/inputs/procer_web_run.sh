#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/chat
# WARNING: on some systems the nohup doesn't work, like OSX
# try running it without
nohup python -u www.py > www.log 2>&1 &
echo $! > $DEPLOY/profiles/web/web.pid

