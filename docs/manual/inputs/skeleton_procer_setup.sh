cd profiles/
ls
# should see: chat  handlertest  mongrel2  mp3stream  web

# make all the restart settings
for i in *; do touch $i/restart; done

# make all the empty dependencies
for i in *; do touch $i/depends; done

# setup the pid_files to some sort of default
for i in *; do echo $PWD/$i/$i.pid > $i/pid_file; done
cat chat/pid_file

# get the run script setup to do nothing
for i in *; do echo '#!/bin/sh' > $i/run; done
for i in *; do chmod u+x $i/run; done

# check out what we did
ls -lR
