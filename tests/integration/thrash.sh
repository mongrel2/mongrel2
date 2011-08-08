while curl http://localhost:6767/ > /dev/null 
do
    echo "CURL TESTS"
    ./tests/integration/curl_tests >> curl_tests.log 

    echo "TNFTP TESTS"
    ./tests/integration/tnftp_tests >> tnftp_tests.log 

    echo "WGET TESTS"
    ./tests/integration/wget_tests >> wget_tests.log 

    echo "POST TESTS"
    ./tests/integration/post_tests >> post_tests.log 

    echo "CHAT TESTS"
    ./tests/integration/chat_tests >> chat_tests.log 

    echo "XML TESTS"
    ./tests/integration/xml_tests >> xml_tests.log

    echo "AND SUITE"
    ./tests/integration/and_tests >> and_tests.log

    echo "RANDOM FUZZ"
    nc localhost 6767 < /dev/urandom

done
