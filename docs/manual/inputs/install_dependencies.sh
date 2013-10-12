# install ZeroMQ
wget http://download.zeromq.org/zeromq-2.1.7.tar.gz
tar -xzvf zeromq-2.1.7.tar.gz
cd zeromq-2.1.7/
./configure
make
sudo make install

# install sqlite3
sudo pacman -S sqlite3

