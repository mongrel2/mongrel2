# run procer to get stuff started
sudo procer $PWD/profiles $PWD/run/procer.pid

# see if it's all running
ps ax | grep procer

# should see:
# 19607 ?        Ss     0:00 procer /home/zedshaw/deployment/profiles /home/zedshaw/deployment/run/procer.pid

ps ax | grep mongrel2

# should see:
# 19621 ?        Ssl    0:00 mongrel2 config.sqlite ba0019c0-9140-4f82-80ca-0f4f2e81def7

ps ax | grep chat

# should see:
# 19609 ?        Sl     0:00 python chat.py

# try killing chat to see if it comes back
kill -TERM `cat profiles/chat/chat.pid`

ps ax | grep chat

# should see:
# 19669 ?        Sl     0:00 python chat.py
