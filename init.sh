if test -d .git && git submodule status| grep '^-'; then git submodule init && git submodule update; fi
cp src/polarssl_config.h src/polarssl/include/polarssl/config.h
