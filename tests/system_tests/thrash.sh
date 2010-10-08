while curl http://localhost:6767/ > /dev/null 
do
    echo "CURL TESTS"
    ./tests/system_tests/curl_tests >> curl_tests.log 

    echo "POST TESTS"
    ./tests/system_tests/post_tests >> post_tests.log 

    echo "CHAT TESTS"
    ./tests/system_tests/chat_tests >> chat_tests.log 

    echo "XML TESTS"
    ./tests/system_tests/xml_tests >> xml_tests.log
done
