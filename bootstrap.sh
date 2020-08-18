#! /bin/sh
 
rm -f config.cache
rm -f config.log

/bin/mkdir -p /usr/local/apie/lib

# aclocal
# autoheader
# autoconf
# libtoolize -f
# automake -a

touch_fils="NEWS README AUTHORS ChangeLog COPING"
touch $touch_fils

autoreconf -i