#include "test.h"
#include "md5.h"
#include "mongo.h"
#include "gridfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#define LARGE 3*1024*1024
#define UPPER 2000*1024
#define LOWER 1024*128
#define DELTA 1024*128

void fill_buffer_randomly( char *data, uint64_t length ) {
    uint64_t i;
    int random;
    char *letters = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int nletters = strlen( letters )+1;

    for ( i = 0; i < length; i++ ) {
        random = rand() % nletters;
        *( data + i ) = letters[random];
    }
}

static void digest2hex( mongo_md5_byte_t digest[16], char hex_digest[33] ) {
    static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i;
    for ( i=0; i<16; i++ ) {
        hex_digest[2*i]     = hex[( digest[i] & 0xf0 ) >> 4];
        hex_digest[2*i + 1] = hex[ digest[i] & 0x0f      ];
    }
    hex_digest[32] = '\0';
}

void test_gridfile( gridfs *gfs, char *data_before, uint64_t length, char *filename, char *content_type ) {
    gridfile gfile[1];
    FILE *fd;
    mongo_md5_state_t pms[1];
    mongo_md5_byte_t digest[16];
    char hex_digest[33];
    uint64_t i = length;
    int n;
    char *data_after = bson_malloc( LARGE );

    gridfs_find_filename( gfs, filename, gfile );
    ASSERT( gridfile_exists( gfile ) );

    fd = fopen( "output", "w+" );
    gridfile_write_file( gfile, fd );
    fseek( fd, 0, SEEK_SET );
    ASSERT( fread( data_after, length, sizeof( char ), fd ) );
    fclose( fd );
    ASSERT( strncmp( data_before, data_after, length ) == 0 );

    gridfile_read( gfile, length, data_after );
    ASSERT( strncmp( data_before, data_after, length ) == 0 );

    ASSERT( strcmp( gridfile_get_filename( gfile ), filename ) == 0 );

    ASSERT( gridfile_get_contentlength( gfile ) == length );

    ASSERT( gridfile_get_chunksize( gfile ) == DEFAULT_CHUNK_SIZE );

    ASSERT( strcmp( gridfile_get_contenttype( gfile ), content_type ) == 0 ) ;

    ASSERT( strncmp( data_before, data_after, length ) == 0 );

    mongo_md5_init( pms );

    n = 0;
    while( i > INT_MAX  ) {
        mongo_md5_append( pms, ( const mongo_md5_byte_t * )data_before + ( n * INT_MAX ), INT_MAX );
        i -= INT_MAX;
        n += 1;
    }
    if( i > 0 )
        mongo_md5_append( pms, ( const mongo_md5_byte_t * )data_before + ( n * INT_MAX ), i );

    mongo_md5_finish( pms, digest );
    digest2hex( digest, hex_digest );
    ASSERT( strcmp( gridfile_get_md5( gfile ), hex_digest ) == 0 );

    gridfile_destroy( gfile );
    gridfs_remove_filename( gfs, filename );
    free( data_after );
    unlink( "output" );
}

void test_basic() {
    mongo conn[1];
    gridfs gfs[1];
    char *data_before = bson_malloc( UPPER );
    uint64_t i;
    FILE *fd;

    srand( time( NULL ) );

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn, TEST_SERVER, 27017 ) ) {
        printf( "failed to connect 2\n" );
        exit( 1 );
    }

    gridfs_init( conn, "test", "fs", gfs );

    fill_buffer_randomly( data_before, UPPER );
    for ( i = LOWER; i <= UPPER; i += DELTA ) {

        /* Input from buffer */
        gridfs_store_buffer( gfs, data_before, i, "input-buffer", "text/html" );
        test_gridfile( gfs, data_before, i, "input-buffer", "text/html" );

        /* Input from file */
        fd = fopen( "input-file", "w" );
        fwrite( data_before, sizeof( char ), i, fd );
        fclose( fd );
        gridfs_store_file( gfs, "input-file", "input-file", "text/html" );
        test_gridfile( gfs, data_before, i, "input-file", "text/html" );
    }

    gridfs_destroy( gfs );
    mongo_disconnect( conn );
    mongo_destroy( conn );
    free( data_before );

    /* Clean up files. */
    unlink( "input-file" );
    unlink( "output" );
}

void test_streaming() {
    mongo conn[1];
    gridfs gfs[1];
    gridfile gfile[1];
    char *buf = bson_malloc( LARGE );
    char *small = bson_malloc( LOWER );
    int n;

    if( buf == NULL || small == NULL ) {
        printf( "Failed to allocate" );
        exit( 1 );
    }

    srand( time( NULL ) );

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect 3\n" );
        exit( 1 );
    }

    fill_buffer_randomly( small, ( uint64_t )LOWER );
    fill_buffer_randomly( buf, ( uint64_t )LARGE );

    gridfs_init( conn, "test", "fs", gfs );

    gridfs_store_buffer( gfs, small, LOWER, "small", "text/html" );
    test_gridfile( gfs, small, LOWER, "small", "text/html" );
    gridfs_destroy( gfs );

    gridfs_init( conn, "test", "fs", gfs );
    gridfile_writer_init( gfile, gfs, "large", "text/html" );
    for( n=0; n < ( LARGE / 1024 ); n++ ) {
        gridfile_write_buffer( gfile, buf + ( n * 1024 ), 1024 );
    }
    gridfile_writer_done( gfile );
    test_gridfile( gfs, buf, LARGE, "large", "text/html" );

    gridfs_destroy( gfs );
    mongo_destroy( conn );
    free( buf );
    free( small );
}

void test_large() {
    mongo conn[1];
    gridfs gfs[1];
    gridfile gfile[1];
    FILE *fd;
    int i, n;
    char *buffer = bson_malloc( LARGE );
    uint64_t filesize = ( uint64_t )1024 * ( uint64_t )LARGE;

    srand( time( NULL ) );

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn, TEST_SERVER, 27017 ) ) {
        printf( "failed to connect 1\n" );
        exit( 1 );
    }

    gridfs_init( conn, "test", "fs", gfs );

    /* Create a very large file */
    fill_buffer_randomly( buffer, ( uint64_t )LARGE );
    fd = fopen( "bigfile", "w" );
    for( i=0; i<1024; i++ ) {
        fwrite( buffer, 1, LARGE, fd );
    }
    fclose( fd );

    /* Now read the file into GridFS */
    gridfs_store_file( gfs, "bigfile", "bigfile", "text/html" );

    gridfs_find_filename( gfs, "bigfile", gfile );

    ASSERT( strcmp( gridfile_get_filename( gfile ), "bigfile" ) == 0 );
    ASSERT( gridfile_get_contentlength( gfile ) ==  filesize );

    /* Read the file using the streaming interface */
    gridfile_writer_init( gfile, gfs, "bigfile-stream", "text/html" );

    fd = fopen( "bigfile", "r" );

    while( ( n = fread( buffer, 1, 1024, fd ) ) != 0 ) {
        gridfile_write_buffer( gfile, buffer, n );
    }
    gridfile_writer_done( gfile );

    gridfs_find_filename( gfs, "bigfile-stream", gfile );

    ASSERT( strcmp( gridfile_get_filename( gfile ), "bigfile-stream" ) == 0 );
    ASSERT( gridfile_get_contentlength( gfile ) ==  filesize );

    gridfs_destroy( gfs );
    mongo_disconnect( conn );
    mongo_destroy( conn );
}

int main( void ) {
    test_basic();
    test_streaming();

    /* Normally not necessary to run test_large(), as it
     * deals with very large (3GB) files and is therefore slow.
     * test_large();
     */
    return 0;
}
