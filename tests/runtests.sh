echo "Running unit tests:"

for i in tests/*_tests
do
    if test -f $i
    then
        if $VALGRIND ./$i 2>&1 > /tmp/mongrel2-test.log
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
