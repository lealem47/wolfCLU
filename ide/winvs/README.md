VisualStudio solution for wolfCLU
=================================

The solution file, wolfclu.sln, facilitates bulding wolfCLU.
The solution provides both Debug and Release
builds of Dynamic 32- or 64-bit libraries. The file
`user_settings.h` should be used in the wolfSSL build to configure it.

The file `wolfclu\ide\winvs\user_settings.h` contains the settings used to
configure wolfSSL with the appropriate settings. This file must be copied
from the directory `wolfclu\ide\winvs` to `wolfssl\IDE\WIN`. The file
`wolfclu\ide\winvs\options.h` should also be copied to 'wolfssl/wolfssl`. You
can then build wolfSSL with support for wolfCLU. 

After building the wolfSSL solution, copy the resulting `wolfssl.lib`
found in either the `Debug` or `Release` directory, into the wolfclu
directory.

Before building wolfCLU, Make sure you have the same architecture (x32 or x64) 
selected as used in wolfSSL.

This project assumes that the wolfSSH and wolfSSL source directories
are installed side-by-side and do not have the version number in their
names:

    Projects\
        wolfclu\
        wolfssl\
