UPLOAD_URL="$(python extract-json.py upload_url < output/release.json)"
RELEASE_ID="$(python extract-json.py id < output/release.json)"
if test -z ${REALLY_RELEASE+x}; then
    cat <<EOF
If REALLY_RELEASE were set then I would run the following:
    curl -u '$(cat $AUTH_FILE)' --data-binary "@output/$TAR_NAME" \
        -H "Content-Type: application/x-bzip2" \
        "${UPLOAD_URL%{*}?name=$TAR_NAME"
EOF
else
    curl -u "$(cat $AUTH_FILE)" --data-binary "@output/$TAR_NAME" \
        -H "Content-Type: application/x-bzip2" \
        "${UPLOAD_URL%{*}?name=$TAR_NAME"
fi
