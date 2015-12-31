DIR="$(pwd)"
if test -e "$CHECKOUT_PATH/output/docs/manual/book-final.html"; then
    echo "Manual appears built, not rebuilding"
else
    echo "Building Manual (output in $DIR/output/build-manual.log)"
    cd "$CHECKOUT_PATH"
    dexy setup
    make manual > "$DIR/output/build-manual.log"
fi
