#include "minunit.h"
#include "stdio.h"
#include "polarssl/x509.h"

char *test_SSL_verify_cert() 
{

    x509_cert crt;
    memset( &crt, 0, sizeof( x509_cert ) );

    x509_cert ca_crt;
    memset( &ca_crt, 0, sizeof( x509_cert ) );

    x509_crl crl;
    memset( &crl, 0, sizeof( x509_crl ) );

    int ret = 0;

    ret =x509parse_crtfile( &crt, "tests/ca/certs/m2-cert.pem" );

    mu_assert(ret == 0, "failed to parse cert m2-cert.pem");

    ret =x509parse_crtfile( &ca_crt, "tests/ca/none.pem" );

    mu_assert(ret != 0, "failed to fail on non-existent pem none.pem");

    ret =x509parse_crtfile( &ca_crt, "tests/ca/cacert.pem" );

    mu_assert(ret == 0, "failed to parse cert cacert.pem");

    ret =x509parse_crlfile( &crl, "tests/ca/crl.pem" );

    mu_assert(ret == 0, "failed to parse cert crl.pem");

    int flags = 0;
    ret =x509parse_verify( &crt, &ca_crt, NULL, NULL, &flags, NULL, NULL);

    mu_assert(ret == 0, "failed to verify cert m2-cert.pem");

    x509_free( &crt );
    x509_free( &ca_crt );
    x509_crl_free( &crl );

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_SSL_verify_cert);

    return NULL;
}

RUN_TESTS(all_tests);

