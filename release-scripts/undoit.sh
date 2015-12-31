DIR="$(dirname "$0")"
git checkout master
git reset --hard "$(cat "$DIR/output/master.commit")"
git checkout develop
git reset --hard "$(cat "$DIR/output/develop.commit")"
