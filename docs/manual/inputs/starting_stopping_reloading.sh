# start it so it runs in the background via sudo
m2sh start -db tests/config.sqlite -host localhost -sudo
tail logs/error.log

# reload it
m2sh reload -db tests/config.sqlite -host localhost
tail logs/error.log

# hit is with curl to see it do the reload
curl http://localhost:6767/
tail logs/error.log

# see if it's running then stop it
m2sh running -db tests/config.sqlite -host localhost
m2sh stop -db tests/config.sqlite -host localhost
