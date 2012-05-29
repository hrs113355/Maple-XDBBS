#!/bin/sh
# $Id: newvers.sh,v 1.2 2003/06/22 14:45:17 in2 Exp $
# modified to use in maple-xdbbs by hrs

# prevent localized logs
LC_ALL=C
export LC_ALL

t=`date`

# are we working in CVS?
if [ -d ".svn" ] ; then

    #determine rev
    rev=`svn info | grep Revision`

    if [ "$rev" != "" ]
    then
        t="Maple-xdbbs ($t, $rev)"
    fi

fi

cat << EOF > /home/bbs/src/maple/vers.c
char    * const compile_time = "${t}";
EOF

