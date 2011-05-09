#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/chat
nohup python -u www.py 2>&1 > www.log &
echo $! > $DEPLOY/profiles/web/web.pid

