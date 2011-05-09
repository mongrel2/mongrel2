cp -r ~/projects/mongrel2/examples/chat/static static/chatdemo
m2sh stop -db config.sqlite -host localhost -murder
curl -I http://localhost:6767/chatdemo/
