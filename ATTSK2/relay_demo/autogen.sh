#!/bin/sh

# autogen.sh -- Autotools bootstrapping
#

export PERL_BADLANG=0
libtoolize --copy --force
aclocal && autoheader && autoconf && automake --add-missing --copy

