/* clu_request_setup.c
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
#include <wolfclu/x509/clu_request.h>
#include <wolfclu/x509/clu_cert.h>
#include <wolfclu/certgen/clu_certgen.h>

#ifdef WOLFSSL_CERT_REQ
static const struct option req_options[] = {

    {"sha",       no_argument,       0, WOLFCLU_CERT_SHA   },
    {"sha224",    no_argument,       0, WOLFCLU_CERT_SHA224},
    {"sha256",    no_argument,       0, WOLFCLU_CERT_SHA256},
    {"sha384",    no_argument,       0, WOLFCLU_CERT_SHA384},
    {"sha512",    no_argument,       0, WOLFCLU_CERT_SHA512},
    {"rsa",       no_argument,       0, WOLFCLU_RSA       },
    {"ed25519",   no_argument,       0, WOLFCLU_ED25519   },

    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"key",       required_argument, 0, WOLFCLU_KEY       },
    {"new",       no_argument,       0, WOLFCLU_NEW       },
    {"inkey",     required_argument, 0, WOLFCLU_INKEY     },
    {"inform",    required_argument, 0, WOLFCLU_INFORM    },
    {"outform",   required_argument, 0, WOLFCLU_OUTFORM   },
    {"config",    required_argument, 0, WOLFCLU_CONFIG },
    {"days",      required_argument, 0, WOLFCLU_DAYS },
    {"x509",      no_argument,       0, WOLFCLU_X509 },
    {"subj",      required_argument, 0, WOLFCLU_SUBJECT },
    {"verify",    no_argument,       0, WOLFCLU_VERIFY },
    {"text",      no_argument,       0, WOLFCLU_TEXT_OUT },
    {"noout",     no_argument,       0, WOLFCLU_NOOUT },
    {"extensions",required_argument, 0, WOLFCLU_EXTENSIONS},

    {0, 0, 0, 0} /* terminal element */
};


#if LIBWOLFSSL_VERSION_HEX < 0x05003000

#define MAX_WIDTH 80
/* print serial number out
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_print_serial(WOLFSSL_BIO* bio, WOLFSSL_X509* x509,
        int indent)
{
    unsigned char serial[32];
    int  sz = sizeof(serial);
    char scratch[MAX_WIDTH];

    XMEMSET(serial, 0, sz);
    if (wolfSSL_X509_get_serial_number(x509, serial, &sz) == WOLFSSL_SUCCESS) {
        if (sz > 1) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s", indent, "",
                    "Serial Number:");
            if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch)) <= 0) {
                return WOLFSSL_FAILURE;
            }

            /* if serial can fit into byte than print on the same line */
            if (sz <= (int)sizeof(byte)) {
                XSNPRINTF(scratch, MAX_WIDTH, " %d (0x%x)\n", serial[0],
                        serial[0]);
                if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch))
                        <= 0) {
                    return WOLFSSL_FAILURE;
                }
            }
            else {
                int i;
                char tmp[100];
                int  tmpSz = 100;
                char val[5];
                int  valSz = 5;

                /* serial is larger than int size so print off hex values */
                XSNPRINTF(scratch, MAX_WIDTH, "\n%*s", indent, "");
                if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch))
                        <= 0) {
                    return WOLFSSL_FAILURE;
                }
                tmp[0] = '\0';
                for (i = 0; i < sz - 1 && (3 * i) < tmpSz - valSz; i++) {
                    XSNPRINTF(val, sizeof(val) - 1, "%02x:", serial[i]);
                    val[3] = '\0'; /* make sure is null terminated */
                    XSTRNCAT(tmp, val, valSz);
                }
                XSNPRINTF(val, sizeof(val) - 1, "%02x\n", serial[i]);
                val[3] = '\0'; /* make sure is null terminated */
                XSTRNCAT(tmp, val, valSz);
                if (wolfSSL_BIO_write(bio, tmp, (int)XSTRLEN(tmp)) <= 0) {
                    return WOLFSSL_FAILURE;
                }
            }
        }
    }
    return WOLFSSL_SUCCESS;
}


/* convert key usage type to human readable print out
 * return WOLFSSL_SUCCESS on success
 */
static int _keyUsagePrint(WOLFSSL_BIO* bio, int keyUsage, int indent)
{
    char scratch[MAX_WIDTH];

    if (keyUsage > 0) {
        if (keyUsage & KEYUSE_KEY_ENCIPHER) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "keyEncipherment");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_DIGITAL_SIG) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "digitalSignature");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_CONTENT_COMMIT) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "nonRepudiation");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_DATA_ENCIPHER) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "dataEncipherment");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_KEY_AGREE) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "keyAgreement");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_KEY_CERT_SIGN) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "", "keyCertSign");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_CRL_SIGN) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "", "cRLSign");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_ENCIPHER_ONLY) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "encipherOnly");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }

        if (keyUsage & KEYUSE_DECIPHER_ONLY) {
            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                    "decipherOnly");
            wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        }
    }

    return WOLFSSL_SUCCESS;
}


/* iterate through certificate extensions printing them out in human readable
 * form
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_extensions_print(WOLFSSL_BIO* bio, WOLFSSL_X509* x509,
        int indent)
{
    char scratch[MAX_WIDTH];
    int count, i;

    count = wolfSSL_X509_get_ext_count(x509);
    if (count > 0) {
        XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent, "",
                "Requested extensions:");
        if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch)) <= 0) {
            return WOLFSSL_FAILURE;
        }

        for (i = 0; i < count; i++) {
            WOLFSSL_X509_EXTENSION* ext = wolfSSL_X509_get_ext(x509, i);
            if (ext != NULL) {
                WOLFSSL_ASN1_OBJECT* obj;
                char buf[MAX_WIDTH-4]; /* -4 to avoid warning when used in
                                        * in XSNPRINTF */
                char* altName;
                int nid;

                obj = wolfSSL_X509_EXTENSION_get_object(ext);
                wolfSSL_OBJ_obj2txt(buf, MAX_WIDTH, obj, 0);
                XSNPRINTF(scratch, MAX_WIDTH, "%*s%s%s\n", indent + 4, "",
                   buf,
                   (wolfSSL_X509_EXTENSION_get_critical(ext))? ": Critical" : ":");

                wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
                nid = wolfSSL_OBJ_obj2nid(obj);
                switch (nid) {
                    case NID_subject_alt_name:
                        while ((altName = wolfSSL_X509_get_next_altname(x509))
                                != NULL) {
                            XSNPRINTF(scratch, MAX_WIDTH, "%*s%s\n", indent + 8,
                                    "", altName);
                            wolfSSL_BIO_write(bio, scratch,
                                    (int)XSTRLEN(scratch));
                        }
                        break;
                    case NID_key_usage:
                        _keyUsagePrint(bio, wolfSSL_X509_get_key_usage(x509),
                                indent + 8);
                        break;
                    default:
                        /* extension nid not yet supported */
                        XSNPRINTF(scratch, MAX_WIDTH,
                                "%*sNID %d print not yet supported\n",
                                indent + 8, "", nid);
                        wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
                }
            }
        }
    }
    return WOLFSSL_SUCCESS;
}


/* @TODO print out of REQ attributes
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_REQ_attributes_print(WOLFSSL_BIO* bio,
        WOLFSSL_X509* x509, int indent)
{
    (void)bio;
    (void)x509;
    (void)indent;
    return WOLFSSL_SUCCESS;
}


/* print out the signature in human readable format for use with
 * wolfSSL_X509_print()
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_signature_print_ex(WOLFSSL_BIO* bio,
        WOLFSSL_X509* x509, int indent)
{
    char scratch[MAX_WIDTH];
    int sigSz;

    wolfSSL_X509_get_signature(x509, NULL, &sigSz);
    if (sigSz > 0) {
        unsigned char* sig;
        int i;
        char tmp[100];
        int sigNid = wolfSSL_X509_get_signature_nid(x509);
        WOLFSSL_ASN1_OBJECT* obj;

        XSNPRINTF(scratch, MAX_WIDTH, "%*s%s", indent, "",
                "Signature Algorithm: ");
        if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch)) <= 0) {
            return WOLFSSL_FAILURE;
        }
        obj = wolfSSL_OBJ_nid2obj(sigNid);
        wolfSSL_OBJ_obj2txt(scratch, MAX_WIDTH, obj, 0);
        wolfSSL_ASN1_OBJECT_free(obj);
        XSNPRINTF(tmp, sizeof(tmp) - 1,"%s\n", scratch);
        tmp[sizeof(tmp) - 1] = '\0';
        if (wolfSSL_BIO_write(bio, tmp, (int)XSTRLEN(tmp)) <= 0) {
            return WOLFSSL_FAILURE;
        }

        sig = (unsigned char*)XMALLOC(sigSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (sig == NULL) {
            return WOLFSSL_FAILURE;
        }

        if (wolfSSL_X509_get_signature(x509, sig, &sigSz) <= 0) {
            XFREE(sig, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            return WOLFSSL_FAILURE;
        }
        XSNPRINTF(tmp, sizeof(tmp) - 1,"        ");
        tmp[sizeof(tmp) - 1] = '\0';
        for (i = 0; i < sigSz; i++) {
            char val[5];
            int valSz = 5;

            if (i == 0) {
                XSNPRINTF(val, valSz - 1, "%02x", sig[i]);
            }
            else if (((i % 18) == 0)) {
                tmp[sizeof(tmp) - 1] = '\0';
                if (wolfSSL_BIO_write(bio, tmp, (int)XSTRLEN(tmp))
                        <= 0) {
                    XFREE(sig, NULL, DYNAMIC_TYPE_TMP_BUFFER);
                    return WOLFSSL_FAILURE;
                }
                XSNPRINTF(tmp, sizeof(tmp) - 1,
                        ":\n        ");
                XSNPRINTF(val, valSz - 1, "%02x", sig[i]);
            }
            else {
                XSNPRINTF(val, valSz - 1, ":%02x", sig[i]);
            }
            XSTRNCAT(tmp, val, valSz);
        }
        XFREE(sig, NULL, DYNAMIC_TYPE_TMP_BUFFER);

        /* print out remaining sig values */
        if ((i > 0) && (((i - 1) % 18) != 0)) {
                tmp[sizeof(tmp) - 1] = '\0';
                if (wolfSSL_BIO_write(bio, tmp, (int)XSTRLEN(tmp))
                        <= 0) {
                    return WOLFSSL_FAILURE;
                }
        }
    }
    return WOLFSSL_SUCCESS;
}


/* print out the public key in human readable format for use with
 * wolfSSL_X509_print()
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_pubkey_print(WOLFSSL_BIO* bio, WOLFSSL_X509* x509,
        int indent)
{
    char scratch[MAX_WIDTH];
    WOLFSSL_EVP_PKEY* pubKey;

    XSNPRINTF(scratch, MAX_WIDTH, "%*sPublic Key:\n", indent, "");
    wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));

    pubKey = wolfSSL_X509_get_pubkey(x509);
    wolfSSL_EVP_PKEY_print_public(bio, pubKey, indent + 4, NULL);
    wolfSSL_EVP_PKEY_free(pubKey);
    return WOLFSSL_SUCCESS;
}


/* human readable print out of x509 name formatted for use with
 * wolfSSL_X509_print()
 * return WOLFSSL_SUCCESS on success
 */
static int _X509_name_print(WOLFSSL_BIO* bio, WOLFSSL_X509_NAME* name,
        char* type, int indent)
{
    char scratch[MAX_WIDTH];
    if (name != NULL) {
        XSNPRINTF(scratch, MAX_WIDTH, "%*s%s", indent, "", type);
        wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch));
        wolfSSL_X509_NAME_print_ex(bio, name, 1, 0);
        wolfSSL_BIO_write(bio, "\n", (int)XSTRLEN("\n"));
    }
    return WOLFSSL_SUCCESS;
}


/* human readable print out of x509 version
 * return WOLFSSL_SUCCESS on success
 */
static int _wolfSSL_X509_version_print(WOLFSSL_BIO* bio, WOLFSSL_X509* x509,
        int indent)
{
    int version;
    char scratch[MAX_WIDTH];

    if ((version = wolfSSL_X509_version(x509)) < 0) {
        return WOLFSSL_FAILURE;
    }

    XSNPRINTF(scratch, MAX_WIDTH, "%*s%s", indent, "", "Version:");
    if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch)) <= 0) {
        return WOLFSSL_FAILURE;
    }

    XSNPRINTF(scratch, MAX_WIDTH, " %d (0x%x)\n", version, (byte)version-1);
    if (wolfSSL_BIO_write(bio, scratch, (int)XSTRLEN(scratch)) <= 0) {
        return WOLFSSL_FAILURE;
    }
    return WOLFSSL_SUCCESS;
}


/* This should work its way into wolfSSL master @TODO
 * For now placing the implementation here so that wolfCLU can be used with
 * the current wolfSSL release.
 * return WOLFSSL_SUCCESS on success
 */
static int wolfSSL_X509_REQ_print(WOLFSSL_BIO* bio, WOLFSSL_X509* x509)
{
    char subjType[] = "Subject: ";

    if (bio == NULL || x509 == NULL) {
        return WOLFSSL_FAILURE;
    }

    if (wolfSSL_BIO_write(bio, "Certificate Request:\n",
                  (int)XSTRLEN("Certificate Request:\n")) <= 0) {
            return WOLFSSL_FAILURE;
    }

    if (wolfSSL_BIO_write(bio, "    Data:\n",
                  (int)XSTRLEN("    Data:\n")) <= 0) {
            return WOLFSSL_FAILURE;
    }

    /* print version of cert */
    if (_wolfSSL_X509_version_print(bio, x509, 8) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    if (_wolfSSL_X509_print_serial(bio, x509, 8) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* print subject */
    if (_X509_name_print(bio, wolfSSL_X509_get_subject_name(x509), subjType, 8)
            != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* get and print public key */
    if (_wolfSSL_X509_pubkey_print(bio, x509, 8) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* print out extensions */
    if (_wolfSSL_X509_extensions_print(bio, x509, 4) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* print out req attributes */
    if (_wolfSSL_X509_REQ_attributes_print(bio, x509, 4) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* print out signature */
    if (_wolfSSL_X509_signature_print_ex(bio, x509, 4) != WOLFSSL_SUCCESS) {
        return WOLFSSL_FAILURE;
    }

    /* done with print out */
    if (wolfSSL_BIO_write(bio, "\n\0", (int)XSTRLEN("\n\0")) <= 0) {
        return WOLFSSL_FAILURE;
    }

    return WOLFSSL_SUCCESS;
}
#endif /* version check on wolfSSL */
#endif

/* return WOLFCLU_SUCCESS on success */
int wolfCLU_requestSetup(int argc, char** argv)
{
#ifndef WOLFSSL_CERT_REQ
    WOLFCLU_LOG(WOLFCLU_E0, "wolfSSL not compiled with --enable-certreq");
     /* silence unused variable warnings */
    (void) argc;
    (void) argv;
    return NOT_COMPILED_IN;
#else
    WOLFSSL_BIO *bioOut = NULL;
    WOLFSSL_BIO *keyIn  = NULL;
    WOLFSSL_BIO *reqIn  = NULL;
    WOLFSSL_X509 *x509  = NULL;
    const WOLFSSL_EVP_MD *md  = NULL;
    WOLFSSL_EVP_PKEY *pkey = NULL;

    int     ret = WOLFCLU_SUCCESS;
    char*   in  = NULL;
    char*   out = NULL;
    char*   config = NULL;
    char*   subj = NULL;
    char*   ext = NULL;

    int     algCheck =   0;     /* algorithm type */
    int     oid      =   0;
    int     outForm = PEM_FORM; /* default to PEM format */
    int     inForm  = PEM_FORM;
    int     option;
    int     longIndex = 1;
    int     days = 0;
    int     genX509 = 0;

    byte doVerify  = 0;
    byte doTextOut = 0;
    byte reSign    = 0; /* flag for if resigning req is needed */
    byte noOut     = 0;

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = wolfCLU_GetOpt(argc, argv, "", req_options,
                    &longIndex )) != -1) {

        switch (option) {
            case WOLFCLU_EXTENSIONS:
                ext = optarg;
                break;

            case WOLFCLU_INFILE:
                reqIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (reqIn == NULL) {
                    WOLFCLU_LOG(WOLFCLU_E0, "Unable to open input file %s",
                            optarg);
                    ret = WOLFCLU_FATAL_ERROR;
                }
                break;

            case WOLFCLU_KEY:
                in = optarg;
                keyIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (keyIn == NULL) {
                    WOLFCLU_LOG(WOLFCLU_E0, "Unable to open public key file %s",
                            optarg);
                    ret = WOLFCLU_FATAL_ERROR;
                }
                break;

            case WOLFCLU_OUTFILE:
                out = optarg;
                break;

            case WOLFCLU_INFORM:
                inForm = wolfCLU_checkInform(optarg);
                break;

            case WOLFCLU_OUTFORM:
                outForm = wolfCLU_checkOutform(optarg);
                break;

            case WOLFCLU_SUBJECT:
                subj = optarg;
                break;

            case WOLFCLU_HELP:
                wolfCLU_certgenHelp();
                return WOLFCLU_SUCCESS;

            case WOLFCLU_RSA:
                algCheck = 1;
                break;

            case WOLFCLU_ED25519:
                algCheck = 2;
                break;

            case WOLFCLU_CONFIG:
                config = optarg;
                break;

            case WOLFCLU_DAYS:
                days = XATOI(optarg);
                break;

            case WOLFCLU_CERT_SHA:
                md  = wolfSSL_EVP_sha1();
                oid = SHA_HASH;
                break;

            case WOLFCLU_CERT_SHA224:
                md  = wolfSSL_EVP_sha224();
                oid = SHA_HASH224;
                break;

            case WOLFCLU_CERT_SHA256:
                md  = wolfSSL_EVP_sha256();
                oid = SHA_HASH256;
                break;

            case WOLFCLU_CERT_SHA384:
                md  = wolfSSL_EVP_sha384();
                oid = SHA_HASH384;
                break;

            case WOLFCLU_CERT_SHA512:
                md  = wolfSSL_EVP_sha512();
                oid = SHA_HASH512;
                break;

            case WOLFCLU_X509:
                genX509 = 1;
                break;

            case WOLFCLU_VERIFY:
                doVerify = 1;
                break;

            case WOLFCLU_TEXT_OUT:
                doTextOut = 1;
                break;

            case WOLFCLU_NOOUT:
                noOut = 1;
                break;

            case WOLFCLU_NEW:
                break;

            case ':':
            case '?':
                WOLFCLU_LOG(WOLFCLU_E0, "Unexpected argument");
                ret = WOLFCLU_FATAL_ERROR;
                wolfCLU_certgenHelp();
                break;

            default:
                WOLFCLU_LOG(WOLFCLU_E0, "Unsupported argument");
                ret = WOLFCLU_FATAL_ERROR;
                wolfCLU_certgenHelp();
        }
    }

    /* default to sha256 if not set */
    if (ret == WOLFCLU_SUCCESS && md == NULL) {
        md  = wolfSSL_EVP_sha256();
        oid = SHA_HASH256;
    }

    if (ret == WOLFCLU_SUCCESS) {
        if (reqIn == NULL) {
            x509 = wolfSSL_X509_new();
            if (x509 == NULL) {
                WOLFCLU_LOG(WOLFCLU_E0, "Issue creating structure to use");
                ret = MEMORY_E;
            }
        }
        else {
            if (inForm == PEM_FORM) {
                wolfSSL_PEM_read_bio_X509_REQ(reqIn, &x509, NULL, NULL);
            }
            else {
                wolfSSL_d2i_X509_REQ_bio(reqIn, &x509);
            }
            if (x509 == NULL) {
                WOLFCLU_LOG(WOLFCLU_E0, "Issue creating structure to use");
                ret = WOLFCLU_FATAL_ERROR;
            }
        }
    }

    if (ret == WOLFCLU_SUCCESS && days > 0) {
        WOLFSSL_ASN1_TIME *notBefore, *notAfter;
        time_t t;

        t = time(NULL);
        notBefore = wolfSSL_ASN1_TIME_adj(NULL, t, 0, 0);
        notAfter = wolfSSL_ASN1_TIME_adj(NULL, t, days, 0);
        if (notBefore == NULL || notAfter == NULL) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error creating not before/after dates");
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            wolfSSL_X509_set_notBefore(x509, notBefore);
            wolfSSL_X509_set_notAfter(x509, notAfter);
        }

        wolfSSL_ASN1_TIME_free(notBefore);
        wolfSSL_ASN1_TIME_free(notAfter);

        reSign = 1; /* re-sign after date change */
    }

    if (ret == WOLFCLU_SUCCESS && keyIn != NULL) {
        pkey = wolfSSL_PEM_read_bio_PrivateKey(keyIn, NULL, NULL, NULL);
        if (pkey == NULL) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error reading key from file");
            ret = USER_INPUT_ERROR;
        }

        if (ret == WOLFCLU_SUCCESS &&
                wolfSSL_X509_set_pubkey(x509, pkey) != WOLFSSL_SUCCESS) {
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    if (ret == WOLFCLU_SUCCESS && keyIn == NULL && reqIn == NULL) {
        WOLFCLU_LOG(WOLFCLU_E0, "Please specify a -key <key> option when "
               "generating a certificate.");
        wolfCLU_certgenHelp();
        ret = USER_INPUT_ERROR;
    }

    if (ret == WOLFCLU_SUCCESS && config != NULL) {
        ret = wolfCLU_readConfig(x509, config, (char*)"req", ext);

        reSign = 1; /* re-sign after config changes */
    }

    if (ret == WOLFCLU_SUCCESS && subj != NULL) {
        WOLFSSL_X509_NAME *name;
        name = wolfCLU_ParseX509NameString(subj, (int)XSTRLEN(subj));
        if (name != NULL) {
            wolfSSL_X509_REQ_set_subject_name(x509, name);
            wolfSSL_X509_NAME_free(name);
        }

        reSign = 1; /* re-sign after subject change */
    }

    /* if no configure is passed in then get input from command line */
    if (ret == WOLFCLU_SUCCESS && subj == NULL && config == NULL &&
            reqIn == NULL) {
        WOLFSSL_X509_NAME *name;

        name = wolfSSL_X509_NAME_new();
        if (name == NULL) {
            ret = MEMORY_E;
        }
        else {
            wolfCLU_CreateX509Name(name);
            wolfSSL_X509_REQ_set_subject_name(x509, name);
            wolfSSL_X509_NAME_free(name);
        }
    }

    /* default to version 1 when generating CSR */
    if (ret == WOLFCLU_SUCCESS) {
        if (wolfSSL_X509_set_version(x509, WOLFSSL_X509_V1) !=
                WOLFSSL_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error setting CSR version");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    /* check that we have the key if re-signing */
    if (ret == WOLFCLU_SUCCESS &&
            (reqIn == NULL || reSign) && pkey == NULL) {
        WOLFCLU_LOG(WOLFCLU_E0, "No key loaded to sign with");
        ret = WOLFCLU_FATAL_ERROR;
    }

    if (ret == WOLFCLU_SUCCESS && bioOut == NULL && out != NULL) {
        bioOut = wolfSSL_BIO_new_file(out, "wb");
        if (bioOut == NULL) {
            WOLFCLU_LOG(WOLFCLU_E0, "Unable to open output file %s",
                    optarg);
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    if (ret == WOLFCLU_SUCCESS && bioOut == NULL) {
        /* output to stdout if no output is provided */
        bioOut = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bioOut != NULL) {
            if (wolfSSL_BIO_set_fp(bioOut, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }
    }

    /* sign the req/cert */
    if (ret == WOLFCLU_SUCCESS && (reqIn == NULL || reSign)) {
        if (genX509) {
            /* default to version 3 which supports extensions */
            if (wolfSSL_X509_set_version(x509, WOLFSSL_X509_V3) !=
                    WOLFSSL_SUCCESS) {
                WOLFCLU_LOG(WOLFCLU_E0, "Unable to set version 3 for cert");
                ret = WOLFSSL_FAILURE;
            }

            if (ret == WOLFCLU_SUCCESS) {
                ret = wolfSSL_X509_sign(x509, pkey, md);
                if (ret > 0)
                    ret = WOLFSSL_SUCCESS;
            }
        }
        else {
            ret = wolfSSL_X509_REQ_sign(x509, pkey, md);
        }

        if (ret != WOLFSSL_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error %d signing", ret);
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            ret = WOLFCLU_SUCCESS;
        }
    }

    if (ret == WOLFCLU_SUCCESS && doVerify) {
        WOLFSSL_EVP_PKEY* pubKey = pkey;

        /* get public key from req if not passed in */
        if (pubKey == NULL) {
            pubKey = wolfSSL_X509_get_pubkey(x509);
        }

        if (pubKey == NULL) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error getting the public key to verify");
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            if (wolfSSL_X509_REQ_verify(x509, pubKey) == 1) {
                WOLFCLU_LOG(WOLFCLU_L0, "verify OK");
            }
            else {
                WOLFCLU_LOG(WOLFCLU_E0, "verify failed");
            }
        }
    }

    if (ret == WOLFCLU_SUCCESS && doTextOut) {
        wolfSSL_X509_REQ_print(bioOut, x509);
    }

    if (ret == WOLFCLU_SUCCESS && !noOut) {
        if (outForm == DER_FORM) {
            if (genX509) {
                ret = wolfSSL_i2d_X509_bio(bioOut, x509);
            }
            else {
                ret = wolfSSL_i2d_X509_REQ_bio(bioOut, x509);
            }
        }
        else {
            if (genX509) {
                ret = wolfSSL_PEM_write_bio_X509(bioOut, x509);
            }
            else {
                ret = wolfSSL_PEM_write_bio_X509_REQ(bioOut, x509);
            }
        }

        if (ret != WOLFSSL_SUCCESS) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error %d writing out cert req", ret);
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            /* set WOLFSSL_SUCCESS case to success value */
            ret = WOLFCLU_SUCCESS;
        }
    }

    (void)algCheck;
    (void)in;
    (void)oid;

    wolfSSL_BIO_free(reqIn);
    wolfSSL_BIO_free(keyIn);
    wolfSSL_BIO_free(bioOut);
    wolfSSL_X509_free(x509);
    wolfSSL_EVP_PKEY_free(pkey);
    return ret;
#endif
}

