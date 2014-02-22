echo "Running unit tests:"

# Run all unit tests, collecting stdout to a file, but letting stderr pass, to
# allow tests to send important messages to the maintainer.
for i in tests/*_tests
do
    if test -f $i
    then
        if $VALGRIND ./$i > /tmp/mongrel2-test.log
        then
            echo $i PASS
        else
            echo "ERROR in test $i:"
            cat /tmp/mongrel2-test.log
            exit 1
        fi
    fi
done

rm -f /tmp/mongrel2-test.log
echo ""
