#!/bin/sh
TMP_NAME="$CHECKOUT_PATH/readme.md.new"
echo "Updating Readme"
awk -f "./update-readme.awk" "$CHECKOUT_PATH/README.md" >$TMP_NAME
mv "$TMP_NAME" "$CHECKOUT_PATH/README.md"
echo "* Updated README URL" >> output/commit_message
