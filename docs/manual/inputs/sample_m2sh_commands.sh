# get list of the available servers to run
m2sh servers -db tests/config.sqlite

# see what hosts a server has
m2sh hosts -db tests/config.sqlite -server test

# find out if a server named 'test' is running
m2sh running -db tests/config.sqlite -name test

# start a server whose default host is 'localhost'
m2sh start -db tests/config.sqlite -host localhost

