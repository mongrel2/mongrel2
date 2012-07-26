# go home first
cd ~/

# create the deployment dir
mkdir deployment
cd deployment/

# fill it with the directories we need
mkdir run tmp logs static profiles

# Note: On some systems zeromq needs access to /proc from
# the chroot - on Linux this command should do it (make
# sure you mount it at boot time as well):
## mkdir -p proc && sudo mount --bind /proc proc

# create the procer profile dirs for each thing
cd profiles/
mkdir chat mp3stream handlertest web mongrel2
cd ..

# copy the mongrel2.conf sample from the source to here
cp ~/mongrel2/examples/configs/mongrel2.conf mongrel2.conf

# setup the mongrel2 database initially
m2sh load

# see our end results
ls

