#include "minunit.h"
#include "stdio.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"
char *test_SSL_verify_cert() 
{

    mbedtls_x509_crt crt;
    memset( &crt, 0, sizeof crt );

    mbedtls_x509_crt ca_crt;
    memset( &ca_crt, 0, sizeof ca_crt );

    mbedtls_x509_crl crl;
    memset( &crl, 0, sizeof crl );

    int ret = 0;

    ret =mbedtls_x509_crt_parse_file( &crt, "tests/ca/certs/m2-cert.pem" );

    mu_assert(ret == 0, "failed to parse cert m2-cert.pem");

    ret =mbedtls_x509_crt_parse_file( &ca_crt, "tests/ca/none.pem" );

    mu_assert(ret != 0, "failed to fail on non-existent pem none.pem");

    ret =mbedtls_x509_crt_parse_file( &ca_crt, "tests/ca/cacert.pem" );

    mu_assert(ret == 0, "failed to parse cert cacert.pem");

    ret =mbedtls_x509_crl_parse_file( &crl, "tests/ca/crl.pem" );

    mu_assert(ret == 0, "failed to parse cert crl.pem");

    /*
     * Validate the cert.  Since these certs are only valid within a certain time period, this test
     * will fail when the current time is outside this period.  To avoid false failures (eg. when
     * building/testing this version of the software in the distant future), adjust the expected
     * test outcome accordingly.  However, log the failure to stderr so that the maintainer can
     * detect the expiry of the cert, and generate/commit a new one from time to time.
     */
    uint32_t flags = 0;
    ret =mbedtls_x509_crt_verify( &crt, &ca_crt, NULL, NULL, &flags, NULL, NULL);
    if ( ret ) {
	char buf[1024];
	buf[0] = 0;
	mbedtls_strerror( ret, buf, sizeof buf );
	fprintf( stderr, "*** x509_crt_verify of m2-cert.pem: %d: %s\n", ret, buf );
    }
    int valid_from = mbedtls_x509_time_is_past( &crt.valid_from );
    int valid_to   = mbedtls_x509_time_is_past( &crt.valid_to );

    int expected = 0;
    if ( valid_from == MBEDTLS_X509_BADCERT_EXPIRED && valid_to == MBEDTLS_X509_BADCERT_EXPIRED ) {
	/*
	 * This cert hasn't yet become active, or has already expired; expect
	 * X509 cert failure (-0x2700)
	 */
	fprintf( stderr, "*** x509_crt_verify WILL FAIL because current data is outside: valid_from '%d/%d/%d %d:%d:%d': %d, valid_to '%d/%d/%d %d:%d:%d': %d\n",
	       crt.valid_from.year, crt.valid_from.mon, crt.valid_from.day, crt.valid_from.hour, crt.valid_from.min, crt.valid_from.sec, valid_from,
	       crt.valid_to  .year, crt.valid_to  .mon, crt.valid_to  .day, crt.valid_to  .hour, crt.valid_to  .min, crt.valid_to  .sec, valid_to );
	fprintf( stderr, "*** If this is the currently supported version, generate and commit a new tests/ca/m2-cert.pem with valid dates\n" );
	expected = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
    }
    mu_assert(ret == expected, "failed to verify cert m2-cert.pem");

    mbedtls_x509_crt_free( &crt );
    mbedtls_x509_crt_free( &ca_crt );
    mbedtls_x509_crl_free( &crl );

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_SSL_verify_cert);

    return NULL;
}

RUN_TESTS(all_tests);

