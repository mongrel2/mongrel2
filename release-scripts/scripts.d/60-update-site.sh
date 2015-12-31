echo "Updating site-header"
TMP_NAME="$(mktemp /tmp/tmp.XXXXXXXX)"
cat <<EOF > "$CHECKOUT_PATH/docs/site/global.json"
{
    "NumericVersion" : "$NUMERIC_VERSION",
    "VersionName" : "$RELEASE_NAME",
    "Owner" : "$OWNER",
    "Repo" : "$REPO",
    "TarName" : "$TAR_NAME",
    "Sha" : "$(set $(sha1sum "output/$TAR_NAME"); echo $1)"
}
EOF
pushd "$CHECKOUT_PATH/docs/site" >/dev/null
make
popd >/dev/null
echo "* Updated README URL" >> output/commit_message
