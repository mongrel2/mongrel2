#include "minunit.h"
#include "stdio.h"
#include "polarssl/x509_crt.h"

char *test_SSL_verify_cert() 
{

    x509_crt crt;
    memset( &crt, 0, sizeof( x509_crt ) );

    x509_crt ca_crt;
    memset( &ca_crt, 0, sizeof( x509_crt ) );

    x509_crl crl;
    memset( &crl, 0, sizeof( x509_crl ) );

    int ret = 0;

    ret =x509_crt_parse_file( &crt, "tests/ca/certs/m2-cert.pem" );

    mu_assert(ret == 0, "failed to parse cert m2-cert.pem");

    ret =x509_crt_parse_file( &ca_crt, "tests/ca/none.pem" );

    mu_assert(ret != 0, "failed to fail on non-existent pem none.pem");

    ret =x509_crt_parse_file( &ca_crt, "tests/ca/cacert.pem" );

    mu_assert(ret == 0, "failed to parse cert cacert.pem");

    ret =x509_crl_parse_file( &crl, "tests/ca/crl.pem" );

    mu_assert(ret == 0, "failed to parse cert crl.pem");

    int flags = 0;
    ret =x509_crt_verify( &crt, &ca_crt, NULL, NULL, &flags, NULL, NULL);

    mu_assert(ret == 0, "failed to verify cert m2-cert.pem");

    x509_crt_free( &crt );
    x509_crt_free( &ca_crt );
    x509_crl_free( &crl );

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_SSL_verify_cert);

    return NULL;
}

RUN_TESTS(all_tests);

