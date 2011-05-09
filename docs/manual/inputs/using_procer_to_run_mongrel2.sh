cd ~/deployment
# clear out the error.log for testing
rm profiles/error.log

# start procer
sudo procer $PWD/profiles $PWD/procer.pid

# see if procer is running
ps ax | grep procer

# should see:
# 17934 ?        Ss     0:00 procer /home/zedshaw/deployment/profiles /home/zedshaw/deployment/procer.pid

# see if mongrel2 is running
ps ax | grep mongrel2

# should see:
# 17944 ?        Ssl    0:00 mongrel2 config.sqlite ba0019c0-9140-4f82-80ca-0f4f2e81def7

