echo "Committing post-release changes"
COMMIT_MSG="$(cat <<EOF
Automated post-release changes for $RELEASE_NAME

$(cat output/commit_message)
EOF
)"
if test -z ${REALLY_RELEASE+x}; then
    cat <<EOF
If REALLY_RELEASE were set then I would run the following:
    git commit -a -m "$COMMIT_MSG"
EOF
else
    git commit -a -m "$COMMIT_MSG"
fi
