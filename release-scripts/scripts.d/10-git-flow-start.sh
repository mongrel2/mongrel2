#!/bin/sh
echo "Starting git flow release"
if	 test -z ${REALLY_RELEASE+x}; then
    cat <<-EOF
	If REALLY_RELEASE were set then I would run the following:
    git flow release start "$RELEASE_NAME"
	EOF
else
    git flow release start "$RELEASE_NAME"
fi
