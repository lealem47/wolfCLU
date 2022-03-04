/* clu_pkcs12.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <wolfclu/clu_header_main.h>
#include <wolfclu/clu_log.h>
#include <wolfclu/clu_optargs.h>
#include <wolfclu/pkey/clu_pkey.h>
#include <wolfclu/x509/clu_cert.h>
#include <wolfclu/x509/clu_parse.h>

static const struct option pkcs12_options[] = {
    {"nodes",     no_argument, 0, WOLFCLU_NODES   },
    {"nocerts",   no_argument, 0, WOLFCLU_NOCERTS },
    {"nokeys",    no_argument, 0, WOLFCLU_NOKEYS  },
    {"passin",    required_argument, 0, WOLFCLU_PASSWORD     },
    {"passout",   required_argument, 0, WOLFCLU_PASSWORD_OUT },
    {"in",        required_argument, 0, WOLFCLU_INFILE       },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE      },
    {"help",      no_argument, 0, WOLFCLU_HELP},
    {"h",         no_argument, 0, WOLFCLU_HELP},

    {0, 0, 0, 0} /* terminal element */
};


static void wolfCLU_pKeyHelp(void)
{
    WOLFCLU_LOG(WOLFCLU_L0, "./wolfssl pkcs12");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-in file input for pkcs12 bundle");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-out file to write results to (default stdout)");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-nodes no DES encryption");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-nocerts no certificate output");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-nokeys no key output");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-passin source to get password from");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-passout source to output password to");
}


int wolfCLU_PKCS12(int argc, char** argv)
{
#ifdef HAVE_PKCS12
    char password[MAX_PASSWORD_SIZE];
    int passwordSz = MAX_PASSWORD_SIZE;
    int ret    = WOLFCLU_SUCCESS;
    int useDES = 1;     /* default to yes */
    int printCerts = 1; /* default to yes*/
    int printKeys  = 1; /* default to yes*/
    int option;
    int longIndex = 1;
    WOLFSSL_EVP_PKEY *pkey = NULL;
    WOLFSSL_X509     *cert = NULL;
    WC_PKCS12        *pkcs12 = NULL;
    WOLF_STACK_OF(WOLFSSL_X509) *extra = NULL;
    WOLFSSL_BIO *bioIn  = NULL;
    WOLFSSL_BIO *bioOut = NULL;

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = wolfCLU_GetOpt(argc, argv, "",
                   pkcs12_options, &longIndex )) != -1) {
        switch (option) {
            case WOLFCLU_NODES:
                useDES = 0;
                break;

            case WOLFCLU_NOCERTS:
                printCerts = 0;
                break;

            case WOLFCLU_NOKEYS:
                printKeys = 0;
                break;

            case WOLFCLU_PASSWORD:
                passwordSz = MAX_PASSWORD_SIZE;
                ret = wolfCLU_GetPassword(password, &passwordSz, optarg);
                break;

            case WOLFCLU_PASSWORD_OUT:
                break;

            case WOLFCLU_INFILE:
                bioIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (bioIn == NULL) {
                    WOLFCLU_LOG(WOLFCLU_E0, "Unable to open pkcs12 file %s",
                            optarg);
                    ret = WOLFCLU_FATAL_ERROR;
                }
                break;

            case WOLFCLU_OUTFILE:
                bioOut = wolfSSL_BIO_new_file(optarg, "wb");
                if (bioOut == NULL) {
                    WOLFCLU_LOG(WOLFCLU_E0, "Unable to open output file %s",
                            optarg);
                    ret = WOLFCLU_FATAL_ERROR;
                }
                break;

            case WOLFCLU_HELP:
                wolfCLU_pKeyHelp();
                return WOLFCLU_SUCCESS;

            case ':':
            case '?':
                WOLFCLU_LOG(WOLFCLU_E0, "Bad argument found");
                wolfCLU_pKeyHelp();
                ret = WOLFCLU_FATAL_ERROR;
                break;

            default:
                /* do nothing. */
                (void)ret;
        }
    }

    /* with currently only supporting PKCS12 parsing, an input file is expected */
    if (ret == WOLFCLU_SUCCESS && bioIn == NULL) {
        WOLFCLU_LOG(WOLFCLU_E0, "No input file set");
        ret = WOLFCLU_FATAL_ERROR;
    }

    /* read the input bio to a temporary buffer and convert to PKCS12
     * wolfSSL_d2i_PKCS12_bio does not yet handle file types */
    if (ret == WOLFCLU_SUCCESS) {
        byte* buf;
        int   bufSz;

        bufSz = wolfSSL_BIO_get_len(bioIn);
        if (bufSz > 0) {
            buf = (byte*)XMALLOC(bufSz, HEAP_HINT, DYNAMIC_TYPE_PKCS);
            if (buf == NULL) {
                ret = WOLFCLU_FATAL_ERROR;
            }
            else {
                /* reading the full file into a buffer */
                if (wolfSSL_BIO_read(bioIn, buf, bufSz) != bufSz) {
                    ret = WOLFCLU_FATAL_ERROR;
                }
                else {
                    pkcs12 = wc_PKCS12_new();
                    if (wc_d2i_PKCS12(buf, bufSz, pkcs12) < 0) {
                        WOLFCLU_LOG(WOLFCLU_E0, "Error reading pkcs12 file");
                        ret = WOLFCLU_FATAL_ERROR;
                    }
                }
                XFREE(buf, HEAP_HINT, DYNAMIC_TYPE_PKCS);
            }
        }
        else {
            WOLFCLU_LOG(WOLFCLU_E0, "Error getting length of pkcs12 file");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    if (ret == WOLFCLU_SUCCESS) {
        if (wolfSSL_PKCS12_parse(pkcs12, password, &pkey, &cert, &extra)
                != WOLFSSL_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error parsing pkcs12 file");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    /* setup output bio to stdout if not already set */
    if (ret == WOLFCLU_SUCCESS && bioOut == NULL) {
        bioOut = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bioOut == NULL) {
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            if (wolfSSL_BIO_set_fp(bioOut, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }
    }

    /* print out the certificate */
    if (ret == WOLFCLU_SUCCESS && cert != NULL && printCerts) {
        if (wolfSSL_PEM_write_bio_X509(bioOut, cert) != WOLFSSL_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error printing cert file");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    /* print out the certificate list */
    if (ret == WOLFCLU_SUCCESS && extra != NULL && printCerts) {
        WOLFSSL_X509 *x509;
        int i;

        for (i = 0; i < wolfSSL_sk_X509_num(extra); i++) {
            x509 = wolfSSL_sk_X509_value(extra, i);
            if (wolfSSL_PEM_write_bio_X509(bioOut, x509) != WOLFSSL_SUCCESS) {
                WOLFCLU_LOG(WOLFCLU_E0, "Error printing cert file");
                ret = WOLFCLU_FATAL_ERROR;
            }
        }
    }

    /* print out the key */
    if (ret == WOLFCLU_SUCCESS && pkey != NULL && printKeys) {
        ret = wolfCLU_pKeyPEMtoPriKey(bioOut, pkey);
        if (ret != WOLFCLU_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error getting pubkey from pem key");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    wolfSSL_BIO_free(bioIn);
    wolfSSL_BIO_free(bioOut);
    wolfSSL_EVP_PKEY_free(pkey);
    wolfSSL_X509_free(cert);
    wolfSSL_sk_X509_pop_free(extra, NULL);
    wc_PKCS12_free(pkcs12);

    (void)useDES;
    return ret;
#else
    WOLFCLU_LOG(WOLFCLU_E0, "Recompile wolfSSL with PKCS12 support");
    return WOLFCLU_FATAL_ERROR;
#endif
}

