m2sh control -every

m2 [test]> help
name  help  
stop  stop the server (SIGINT)  
reload  reload the server  
help  this command  
control_stop  stop control port  
kill  kill a connection  
status  status, what=['net'|'tasks']  
terminate  terminate the server (SIGTERM)  
time  the server's time  
uuid  the server's uuid  
info  information about this server  

m2 [test]> info
port:  6767
bind_addr:  0.0.0.0
uuid:  f400bf85-4538-4f7a-8908-67e313d515c2
chroot:  ./
access_log:  .//logs/access.log
error_log:  /logs/error.log
pid_file:  ./run/mongrel2.pid
default_hostname:  localhost

m2 [test]> 

