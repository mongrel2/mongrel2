./profiles/chat/run
nohup: redirecting stderr to stdout
# you'll only see the above if you needed nohup

ps ax | grep chat

# should see: 19305 pts/1    Sl     0:00 python chat.py

kill -TERM 19305
