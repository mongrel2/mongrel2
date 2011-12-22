set -e

while true 
do 
    m2sh reload -every 
    curl http://localhost:6767/tests/sample.html 
    curl http://localhost:6767/handlertest 
done
