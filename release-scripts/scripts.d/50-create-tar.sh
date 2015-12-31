#/bin/sh
#set -x
echo "Creating tarball"
NEW_DIR="$(mktemp -d /tmp/tmp.XXXXXXXX)"
OLD_DIR="$(pwd)"
pushd $NEW_DIR >/dev/null
CLONE_NAME="${TAR_NAME%.tar.bz2}"
git clone --quiet "$CHECKOUT_PATH/.git" "$CLONE_NAME"
cd "$CLONE_NAME"

if test -z ${REALLY_RELEASE+x}; then
    echo "Not putting submodule in tarball to save time..."
else
    git submodule update --init
fi

rm -rf .git
cd ..
tar c "$CLONE_NAME"|bzip2 -9 > "$OLD_DIR/output/$TAR_NAME"
popd >/dev/null
rm -rf -- "$NEW_DIR"
