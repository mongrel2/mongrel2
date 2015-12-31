echo "Updating version.h"
cat <<EOF > $CHECKOUT_PATH/src/version.h
#define VERSION "Mongrel2/$NUMERIC_VERSION"
EOF

echo "* Updated version header file" >> output/commit_message
