#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/http_0mq
# WARNING: on some systems the nohup doesn't work, like OSX
# try running it without
nohup python -u http.py > http.log 2>&1 &
echo $! > $DEPLOY/profiles/handlertest/handlertest.pid

