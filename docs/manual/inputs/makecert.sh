# make a certs directory
mkdir certs

# list out your servers so you can get the UUID
m2sh servers

# go into the certs directory
cd certs

# make a self-signed weak cert to play with
openssl genrsa -des3 -out server.key 512
openssl req -new -key server.key -out server.csr
cp server.key server.key.org
openssl rsa -in server.key.org -out server.key
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

# finally, copy the sesrver.crt and server.key files over to the UUID for that
# server configuration in your mongrel2.conf
mv server.crt 2f62bd5-9e59-49cd-993c-3b6013c28f05.crt
mv server.key 2f62bd5-9e59-49cd-993c-3b6013c28f05.key

