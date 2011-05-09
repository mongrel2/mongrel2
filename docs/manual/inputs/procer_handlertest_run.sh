#!/bin/sh
set -e

DEPLOY=/home/YOU/deployment
SOURCE=/home/YOU/projects/mongrel2

cd $SOURCE/examples/http_0mq
nohup python -u http.py 2>&1 > http.log &
echo $! > $DEPLOY/profiles/handlertest/handlertest.pid

