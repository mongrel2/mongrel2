
for i in tests/*_tests
do
    if test -f $i
    then
        ./$i
    fi
done

