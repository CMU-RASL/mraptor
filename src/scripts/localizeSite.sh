#!/bin/bash
# Suck down the public site, and turn it into a static locally-viewable copy

# Pull down pages; this resolves all the PHP foo.
mkdir mraptor
cd mraptor
wget --quiet -nH -r -p -k -l inf --cut-dirs=1 --reject '*.rpm,*.tar.gz,*.deb' http://www.microraptor.org
cd ..

# Question marks don't do well. :-)
rename 'html?page' 'html.page' mraptor/index*
find mraptor/ -name '*index.html.*' -exec perl -i -p -e 's/html\?page/html\.page/' \{\} \;
# Top-level index.html not caught by above pattern.
find mraptor/ -name '*index.html' -exec perl -i -p -e 's/html\?page/html\.page/' \{\} \;

# We don't want the search results to point to the webserver
perl -i -p -e 's/(urlprefix\s*=\s*\").*(\")/$1\.\/$2/' mraptor/search.js
perl -i -p -e 's/pageref\s*=\s*\"index\.html\?page=\"/pageref = \"index\.html\.page=\"/' mraptor/search.js

# Add a footer to the bottom of each page, pointing to www.microraptor.org
find mraptor/ -name '*index.html.*' -exec perl -i -p -e 's/(Last modified:.*?)<\/td>/<table><tr><td align="left">$1<\/td><td align="right"><a href="http:\/\/www\.microraptor\.org">Microraptor on the Web<\/a><\/td><\/tr><\/table>\n    <\/td>/' \{\} \;
find mraptor/ -name '*index.html' -exec perl -i -p -e 's/(Last modified:.*?)<\/td>/<table width="95%"><tr><td align="left">$1<\/td><td align="right"><a href="http:\/\/www\.microraptor\.org">Latest Release | Microraptor Site<\/a><\/td><\/tr><\/table>\n    <\/td>/' \{\} \;

# Add the version number to our title image
if [ -n "$1" ]; then
    VERSTR="\"ver. $1\"";
    mogrify -fill "#032677" -font Helvetica -pointsize 16 -draw "text 260,15 ${VERSTR} " mraptor/title.png
fi

mv mraptor site_local
