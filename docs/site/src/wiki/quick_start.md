Getting Started With Mongrel2
=============================

This is the fastest way to get started with Mongrel2.  Nothing is really
explained, just all the stuff you need is setup and you get to run a few
commands.  If you want very good explanations for all of this, go read [The
Mongrel2 Manual](http://mongrel2.org/manual/book-final.html) a complete
manual covering everything from getting started, to writing your first
handlers.

This getting started assumes you know what you're doing and can run commands in Unix.

Building The Dependencies
----------

Here's how I might do it on ArchLinux:

<pre>
# install ZeroMQ 
wget http://download.zeromq.org/zeromq-2.1.7.tar.gz 
tar -xzvf zeromq-2.1.7.tar.gz 
cd zeromq-2.1.7/ 
./configure 
make 
sudo make install 
 
# install sqlite3 
sudo pacman -S sqlite3 
</pre>


Getting The Source
------------------

Quickest way to do that is to grab the tarball

<pre>
wget https://github.com/zedshaw/mongrel2/releases/download/v1.9.0/mongrel2-v1.9.0.tar.gz
</pre>


Building Mongrel2
-----------------

Now you need to build mongrel2:

<pre>
tar -xzvf mongrel2-v1.9.0.tar.gz
cd mongrel2-v1.9.0/
make clean all && sudo make install
</pre>

The version number for your directory might be different since we 
update it frequently.


Configuring The First Time
------------------------------

Now you can try out the simplest config example and get it running:

<pre>
cp examples/configs/sample.conf mysite.conf
m2sh load -config mysite.conf
ls config.sqlite
</pre>

*NOTE:* There's also other examples in examples/*.conf.

Running Mongrel2
----------------

Now you can run this and try it.  Make sure you're still in the mongrel2 source directory:

<pre>
mkdir run logs tmp
m2sh start -host localhost
</pre>

From another window do:

<pre>
curl http://localhost:6767/tests/sample.html
hi there
</pre>


Shutting Down
-------------

Just do CTRL-C and it'll exit.  *m2sh* has many other commands and some of them
shut things down or restart.  Run *m2sh help* to find out more.



Learn More From The Manual
-------------------------

That is the fastest crash course you can get in running Mongrel2.  You
should now go read <a href="http://mongrel2.org/manual/book-final.html">The Mongrel2 Manual
 (HTML)</a> which we took much more time writing and making very nice for
you.

