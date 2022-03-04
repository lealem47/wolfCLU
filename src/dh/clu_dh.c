/* clu_dh.c
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

#ifndef NO_DH

#ifndef WOLFSSL_MAX_DH_BITS
    #define WOLFSSL_MAX_DH_BITS       4096
#endif

#ifndef WOLFSSL_MAX_DH_Q_SIZE
    #define WOLFSSL_MAX_DH_Q_SIZE     256
#endif

static const struct option dh_options[] = {
    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"genkey",    no_argument,       0, WOLFCLU_GEN_KEY   },
    {"check",     no_argument,       0, WOLFCLU_CHECK     },
    {"noout",     no_argument,       0, WOLFCLU_NOOUT     },
    {"help",      no_argument,       0, WOLFCLU_HELP      },
    {"h",         no_argument,       0, WOLFCLU_HELP      },

    {0, 0, 0, 0} /* terminal element */
};


static void wolfCLU_DhHelp(void)
{
    WOLFCLU_LOG(WOLFCLU_L0, "./wolfssl dhparam");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-in file input for key to read");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-out file to output to (default stdout)");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-genkey generate DH key using param input");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-check  check if parameters are valid");
    WOLFCLU_LOG(WOLFCLU_L0, "\t-noout  do not print out the params");
}
#endif /* !NO_DH */


int wolfCLU_DhParamSetup(int argc, char** argv)
{
#ifndef NO_DH
    WC_RNG rng;
    DhKey dh;
    int modSz;
    int ret    = WOLFCLU_SUCCESS;
    int option;
    int longIndex = 1;
    char* out = NULL;
    byte genKey = 0;
    byte check  = 0;
    byte noOut  = 0;
    WOLFSSL_BIO *bioIn  = NULL;
    WOLFSSL_BIO *bioOut = NULL;

    /* last parameter is the dh size */
    if (XSTRNCMP("-h", argv[argc-1], 2) == 0) {
        wolfCLU_DhHelp();
        return WOLFCLU_SUCCESS;
    }
    else {
        modSz = XATOI(argv[argc-1]);
        if (modSz <= 0) {
            /* hold off on error'ing out in case there is '-in' used */
        }
    }

    optind = 0; /* start at indent 0 */
    while ((option = wolfCLU_GetOpt(argc, argv, "",
                   dh_options, &longIndex )) != -1) {
        switch (option) {
            case WOLFCLU_INFILE:
                bioIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (bioIn == NULL) {
                    WOLFCLU_LOG(WOLFCLU_E0, "Unable to open input file %s",
                            optarg);
                    ret = WOLFCLU_FATAL_ERROR;
                }
                break;

            case WOLFCLU_OUTFILE:
                out = optarg;
                break;

            case WOLFCLU_GEN_KEY:
                genKey = 1;
                break;
            
            case WOLFCLU_CHECK:
                check = 1;
                break;

            case WOLFCLU_NOOUT:
                noOut = 1;
                break;

            case WOLFCLU_HELP:
                wolfCLU_DhHelp();
                return WOLFCLU_SUCCESS;

            case ':':
            case '?':
                WOLFCLU_LOG(WOLFCLU_E0, "Bad argument");
                ret = USER_INPUT_ERROR;
                break;

            default:
                WOLFCLU_LOG(WOLFCLU_E0, "Bad argument");
                ret = USER_INPUT_ERROR;
        }
    }

    /* try initializing both because both get free'd regardless at the end */
    if (wc_InitRng(&rng) != 0 || wc_InitDhKey(&dh) != 0) {
        WOLFCLU_LOG(WOLFCLU_E0, "Unable to initialize rng and dh");
        ret = WOLFCLU_FATAL_ERROR;
    }

    /* read in parameters */
    if (ret == WOLFCLU_SUCCESS && bioIn != NULL) {
        DerBuffer* pDer = NULL;
        byte* in = NULL;
        word32 inSz = 0;
        word32 idx  = 0;

        inSz = wolfSSL_BIO_get_len(bioIn);
        if (inSz > 0) {
            in = (byte*)XMALLOC(inSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            if (in == NULL) {
                ret = WOLFCLU_FATAL_ERROR;
            }

            if (ret == WOLFCLU_SUCCESS &&
                    wolfSSL_BIO_read(bioIn, in, inSz) <= 0) {
                ret = WOLFCLU_FATAL_ERROR;
            }

            if (ret == WOLFCLU_SUCCESS &&
                    wc_PemToDer(in, inSz, DH_PARAM_TYPE, &pDer, NULL, NULL,
                        NULL) != 0) {
                ret = WOLFCLU_FATAL_ERROR;
            }

            /* der should always be smaller then pem but check just in case */
            if (ret == WOLFCLU_SUCCESS && inSz < pDer->length) {
                ret = WOLFCLU_FATAL_ERROR;
            }

            if (ret == WOLFCLU_SUCCESS) {
                inSz = pDer->length;
                XMEMCPY(in, pDer->buffer, pDer->length);
            }

            if (ret == WOLFCLU_SUCCESS &&
                    wc_DhKeyDecode(in, &idx, &dh, inSz) != 0) {
                WOLFCLU_LOG(WOLFCLU_E0, "Unable to decode input params");
                ret = WOLFCLU_FATAL_ERROR;
            }

            if (in != NULL)
                XFREE(in, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            if (pDer != NULL)
                wc_FreeDer(&pDer);
        }
    }

    if (ret == WOLFCLU_SUCCESS) {
        if (out != NULL) {
            bioOut = wolfSSL_BIO_new_file(out, "wb");
            if (bioOut == NULL) {
                WOLFCLU_LOG(WOLFCLU_E0, "Unable to open output file %s",
                        optarg);
                ret = WOLFCLU_FATAL_ERROR;
            }
        }
        else {
            /* use stdout by default */
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
    }

    /* generate the dh parameters */
    if (ret == WOLFCLU_SUCCESS && bioIn == NULL) {
        if (wc_DhGenerateParams(&rng, modSz, &dh) != 0) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error generating parameters");
            ret = WOLFCLU_FATAL_ERROR;
        }
    }

    /* print out the dh parameters */
    if (ret == WOLFCLU_SUCCESS && !noOut) {
        byte* outBuf = NULL;
        byte* pem    = NULL;
        word32 outBufSz = 0;
        int pemSz       = 0;

        if (wc_DhParamsToDer(&dh, NULL, &outBufSz) != LENGTH_ONLY_E) {
            WOLFCLU_LOG(WOLFCLU_E0, "Unable to get output buffer size");
            ret = WOLFCLU_FATAL_ERROR;
        }

        if (ret == WOLFCLU_SUCCESS) {
            outBuf = (byte*)XMALLOC(outBufSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            if (outBuf == NULL) {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }

        if (ret == WOLFCLU_SUCCESS &&
                wc_DhParamsToDer(&dh, outBuf, &outBufSz) <= 0) {
            ret = WOLFCLU_FATAL_ERROR;
        }

        if (ret == WOLFCLU_SUCCESS) {
            pemSz = wc_DerToPem(outBuf, outBufSz, NULL, 0, DH_PARAM_TYPE);
            if (pemSz > 0) {
                pem = (byte*)XMALLOC(pemSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
                if (pem == NULL) {
                    ret = WOLFCLU_FATAL_ERROR;
                }
            }
            else {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }

        if (ret == WOLFCLU_SUCCESS) {
            pemSz = wc_DerToPem(outBuf, outBufSz, pem, pemSz, DH_PARAM_TYPE);
            if (pemSz <= 0) {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }

        if (ret == WOLFCLU_SUCCESS &&
                wolfSSL_BIO_write(bioOut, pem, pemSz) <= 0) {
            ret = WOLFCLU_FATAL_ERROR;
        }

        if (pem != NULL)
            XFREE(pem, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (outBuf != NULL)
            XFREE(outBuf, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
   
     /* Check if parameters are valid */
    if(ret == WOLFCLU_SUCCESS && check){
        byte p[WOLFSSL_MAX_DH_BITS/8];
        byte g[WOLFSSL_MAX_DH_BITS/8];
        byte q[WOLFSSL_MAX_DH_Q_SIZE/8];
        word32 p_len, g_len, q_len;

        p_len = (word32)sizeof(p);
        q_len = (word32)sizeof(q);
        g_len = (word32)sizeof(g);
        
        /* Export DH parameters */
        if (wc_DhExportParamsRaw(&dh, p, &p_len, q, &q_len, g, &g_len) != 0) {
            WOLFCLU_LOG(WOLFCLU_E0, "Failed to export DH params");
            ret = WOLFCLU_FATAL_ERROR;
        }

        if (wc_DhSetCheckKey(&dh, p, p_len, g, g_len, q, q_len, 0, &rng) != 0) {
            WOLFCLU_LOG(WOLFCLU_E0, "Failed to set/check DH params");
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            WOLFCLU_LOG(WOLFCLU_L0, "DH params are valid.");
        }

     }

    /* print out the dh key */
    if (ret == WOLFCLU_SUCCESS && genKey) {
        byte priv[WOLFSSL_MAX_DH_BITS/8];
        byte pub[WOLFSSL_MAX_DH_BITS/8]; 
        word32 privSz   = (word32)sizeof(priv);
        word32 pubSz    = (word32)sizeof(pub);        
        byte* outBuf    = NULL;
        byte* pem       = NULL;
        word32 outBufSz = 0;
        word32 pemSz    = 0;

        if (wc_DhGenerateKeyPair(&dh, &rng, priv, &privSz, pub, &pubSz) != 0) {
            WOLFCLU_LOG(WOLFCLU_E0, "Error making DH key");
            ret = WOLFCLU_FATAL_ERROR;
        }

        /* get DER size (param has p,q,g and key has p,q,g,y,x) */
        if (wc_DhParamsToDer(&dh, NULL, &outBufSz) != LENGTH_ONLY_E) {
            WOLFCLU_LOG(WOLFCLU_E0, "Unable to get output buffer size");
            ret = WOLFCLU_FATAL_ERROR;
        }
        else {
            ret = WOLFCLU_SUCCESS;
        }

        if (ret == WOLFCLU_SUCCESS) {
            /* size is p,q,g + x,y
             * x will be q size plus 64 bits
             * y will be result of g^x mod p */
            outBufSz = outBufSz + outBufSz + (64/WOLFSSL_BIT_SIZE);
            outBuf = (byte*)XMALLOC(outBufSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            if (outBuf == NULL) {
                ret = WOLFCLU_FATAL_ERROR;
            }

        }

        if (ret == WOLFCLU_SUCCESS) {
            ret = wc_DhPrivKeyToDer(&dh, outBuf, &outBufSz);
            if (ret <= 0) {
                WOLFCLU_LOG(WOLFCLU_E0, "Error converting DH key to buffer");
                ret = WOLFCLU_FATAL_ERROR;
            }
            else {
                outBufSz = (word32)ret;
                ret = WOLFCLU_SUCCESS;
            }
        }

        if (ret == WOLFCLU_SUCCESS) {
            pemSz = wc_DerToPem(outBuf, outBufSz, NULL, 0, DH_PRIVATEKEY_TYPE);
            if (pemSz > 0) {
                pem = (byte*)XMALLOC(pemSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
                if (pem == NULL) {
                    ret = WOLFCLU_FATAL_ERROR;
                }
            }
            else {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }

        if (ret == WOLFCLU_SUCCESS) {
            pemSz = wc_DerToPem(outBuf, outBufSz, pem, pemSz,
                    DH_PRIVATEKEY_TYPE);
            if (pemSz <= 0) {
                ret = WOLFCLU_FATAL_ERROR;
            }
        }

        if (ret == WOLFCLU_SUCCESS &&
                wolfSSL_BIO_write(bioOut, pem, pemSz) <= 0) {
            ret = WOLFCLU_FATAL_ERROR;
        }

        if (pem != NULL)
            XFREE(pem, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (outBuf != NULL)
            XFREE(outBuf, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }

    wolfSSL_BIO_free(bioIn);
    wolfSSL_BIO_free(bioOut);

    wc_FreeDhKey(&dh);
    wc_FreeRng(&rng);

    return ret;
#else
    (void)argc;
    (void)argv;
    WOLFCLU_LOG(WOLFCLU_E0, "DH support not compiled into wolfSSL");
    return WOLFCLU_FATAL_ERROR;
#endif
}


