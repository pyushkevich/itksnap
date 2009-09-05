#!/bin/sh

#
# CREATE RELEASE TAR FILE
#     For maintainer only!
#

VERSION=`awk '/^[0-9]*\.[^ \t]/ {print $1; exit(0);}'<CHANGES`
TARFILE=/var/tmp/Fl_Native_File_Chooser-$VERSION.tar.gz
RELEASEDIR=./.release/Fl_Native_File_Chooser-$VERSION

# MODES
chmod 644 FL/* fltk/* *.cxx [A-Z]* make.bat \
	  documentation/*.html documentation/images/* \
	  reference/* \
	  Makefile* CHANGES CREDITS TODO README*

chmod 755 reference documentation documentation/images \
	  FL fltk

# RELEASE DIR
rm -rf ./.release/ 2> /dev/null
mkdir -p $RELEASEDIR

cp -rp .TAR_RELEASE.sh * ./.release/Fl_Native_File_Chooser-$VERSION

sleep 1		# prevents 'file changed' during tar

# Remove local settings
awk '/REMOVE:START/ { remove = 1; }
     /REMOVE:END/   { remove = 0; }
     { if ( remove == 0 ) print $0 }' \
	   < $RELEASEDIR/Makefile \
	   > $RELEASEDIR/Makefile.new
mv $RELEASEDIR/Makefile.new $RELEASEDIR/Makefile

# CREATE TAR FILE
( cd ./.release; \
  tar cvfz $TARFILE --numeric-owner \
                    --owner=0 \
		    --group=0 \
		    --exclude=scp-to-seriss \
		    Fl_Native_File_Chooser-$VERSION 
)

# CLEANUP
rm -rf ./.release 2> /dev/null
echo "*** Created: $TARFILE"

# UPLOAD
if [ -x ./scp-to-seriss ]; then ./scp-to-seriss $TARFILE; fi

exit 0
