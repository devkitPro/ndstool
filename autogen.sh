cd $(dirname $0)
aclocal
autoconf
automake --add-missing -c
