#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *db = "test";
static const char *ns = "test.c.error";

int main() {
    mongo conn[1];
    bson obj;

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }


    /*********************/
    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( conn->lasterrcode == 0 );
    ASSERT( conn->lasterrstr == NULL );

    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( conn->lasterrcode == 0 );
    ASSERT( conn->lasterrstr == NULL );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    /*********************/
    mongo_simple_int_command( conn, db, "forceerror", 1, NULL );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_ERROR );
    ASSERT( conn->lasterrcode == 10038 );
    ASSERT( strcmp( ( const char * )conn->lasterrstr, "forced error" ) == 0 );

    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_ERROR );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    /* should clear lasterror but not preverror */
    mongo_find_one( conn, ns, bson_empty( &obj ), bson_empty( &obj ), NULL );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_ERROR );
    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    /*********************/
    mongo_cmd_reset_error( conn, db );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    mongo_cmd_drop_db( conn, db );
    mongo_destroy( conn );

    return 0;
}
