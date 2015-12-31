#/bin/sh
RELEASE_INFO="$(cat <<EOF
{
    "tag_name" : "$RELEASE_NAME",
    "draft" : true
}
EOF
)"

if test -z ${REALLY_RELEASE+x}; then
    cp dummy-release.json output/release.json
    cat <<EOF
If REALLY_RELEASE were set then I would run the following:
    git push origin "$RELEASE_NAME"
    git push origin develop
    curl -u "$(cat $AUTH_FILE)" --data "$RELEASE_INFO" \
         "https://api.github.com/repos/$OWNER/$REPO/releases" > output/release.json
EOF
else
    git push origin "$RELEASE_NAME"
    git push origin develop
    git push origin master
    curl -u "$(cat $AUTH_FILE)" --data "$RELEASE_INFO" \
         "https://api.github.com/repos/$OWNER/$REPO/releases" > output/release.json
fi
