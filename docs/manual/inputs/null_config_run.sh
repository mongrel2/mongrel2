mongrel2 goodpath 2f62bd5-9e59-49cd-993c-3b6013c28f05 /usr/local/lib/mongrel2/config_modules/null.so 

# OUTPUT:
#[INFO] (src/mongrel2.c:320) Using configuration module /usr/local/lib/mongrel2/config_modules/null.so to load configs.
#[INFO] (null.c:11) Got the good path.
#[ERROR] (src/config/config.c:366: errno: None) Wrong type, expected valid rows.
#[ERROR] (src/mongrel2.c:124: errno: None) Failed to load global settings.
#[ERROR] (src/mongrel2.c:326: errno: None) Aborting since can't load server.
#[ERROR] (src/mongrel2.c:362: errno: None) Exiting due to error.

mongrel2 badpath 2f62bd5-9e59-49cd-993c-3b6013c28f05 /usr/local/lib/mongrel2/config_modules/null.so 

#[INFO] (src/mongrel2.c:320) Using configuration module /usr/local/lib/mongrel2/config_modules/null.so to load configs.
#[INFO] (null.c:14) Got the bad path: badpath
#[ERROR] (src/mongrel2.c:121: errno: None) Failed to load config database at badpath
#[ERROR] (src/mongrel2.c:326: errno: None) Aborting since can't load server.
#[ERROR] (src/mongrel2.c:362: errno: None) Exiting due to error.


