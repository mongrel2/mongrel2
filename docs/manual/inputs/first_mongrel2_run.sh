mkdir run
mkdir logs
mkdir tmp
m2sh servers -db tests/config.sqlite
m2sh start -db tests/config.sqlite -host localhost
