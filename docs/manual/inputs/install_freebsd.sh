# install ZeroMQ
cd /usr/ports/devel/zmq
make
make install

# install sqlite3
cd /usr/ports/databases/sqlite3
make
make install

# install mongrel2
cd /where/you/extracted
gmake freebsd install
