set -x
NEWDIR=$(mktemp -d /tmp/tmp.XXXXXXXX)
OLDDIR=$(pwd)
cd $NEWDIR
git clone $OLDDIR/.git $1
cd $1
git submodule init
git submodule update
rm -rf .git
cd ..
tar c $1|bzip2 -9 > $OLDDIR/$1.tar.bz2
cd $OLDDIR
rm -rf $NEWDIR -- mongrel2-${VERSION}
