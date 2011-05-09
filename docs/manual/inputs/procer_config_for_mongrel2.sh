cd ~/deployment
# make mongrel2 run as root
sudo chown root.root profiles/mongrel2

# tell procer where mongrel2 puts its pid_file
# notice the > not >> on this
echo "$PWD/run/mongrel2.pid" > profiles/mongrel2/pid_file

# make the run script start mongrel2 (notice the >> on this)
echo "cd $PWD" >> profiles/mongrel2/run
echo "m2sh start -db config.sqlite -host localhost" >> profiles/mongrel2/run

# check out the results
cat profiles/mongrel2/run
#!/bin/sh
cd /home/YOU/deployment
m2sh start -db config.sqlite -host localhost
