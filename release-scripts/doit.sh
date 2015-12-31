set -e
. ./release-config.sh
cd "$(dirname "$0")"
rm -rf output
mkdir output
git rev-parse develop > output/develop.commit
git rev-parse master > output/master.commit

if test -z ${REALLY_RELEASE+x}; then
    echo 'AUTH_INFO' > output/dummy_auth
    AUTH_FILE="$(pwd)/output/dummy_auth"
fi

for script in scripts.d/*; do
    sh -e $script
done
