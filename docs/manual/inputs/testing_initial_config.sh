m2sh start -db config.sqlite -host localhost
# hit ^C to exit out
m2sh start -db config.sqlite -host localhost -sudo
less logs/error.log
m2sh stop -db config.sqlite -host localhost -murder
