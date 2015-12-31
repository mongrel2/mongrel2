COMMIT_MSG="$(cat <<EOF
Automated release changes for $RELEASE_NAME

$(cat output/commit_message)
EOF
)"
rm output/commit_message
echo "Finishing git flow release"
if test -z ${REALLY_RELEASE+x}; then
    cat <<EOF
If REALLY_RELEASE were set then I would run the following:
    git commit -a -m "$COMMIT_MSG"
    git flow release finish "$RELEASE_NAME"
EOF
else
    git commit -a -m "$COMMIT_MSG"
    git flow release finish "$RELEASE_NAME"
fi
