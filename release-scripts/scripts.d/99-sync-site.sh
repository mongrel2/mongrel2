echo "Syncing site"
NEW_DIR="$(mktemp -d /tmp/tmp.XXXXXXXX)"
pushd $NEW_DIR >/dev/null
git clone "https://github.com/$OWNER/$SITE_REPO.git"
cd "$SITE_REPO"
rsync -r --exclude .git --delete "$CHECKOUT_PATH/docs/site/output/" "./"
git add .
git commit -m "Update site for $RELEASE_NAME"
if test -z ${REALLY_RELEASE+x}; then
    cat <<EOF
If REALLY_RELEASE were set then I would run the following:
    git push origin master
EOF
else
    git push origin gh-pages
fi
popd >/dev/null
rm -rf $NEW_DIR
