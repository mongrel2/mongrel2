#!/bin/sh
export REPO=mongrel2
export NUMERIC_VERSION=1.12.0
export OWNER=mongrel2
export REPO=mongrel2
export SITE_REPO=website
export CHECKOUT_PATH="$(pwd)"
export AUTH_FILE="${1:?Must specify curl authentication file}"
export REALLY_RELEASE=yes

# Lines below here typically shouldn't need changing
export RELEASE_NAME="v$NUMERIC_VERSION"
export TAR_NAME="$REPO-$RELEASE_NAME.tar.bz2"
