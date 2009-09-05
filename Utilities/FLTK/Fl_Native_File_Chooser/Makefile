#
# Fl_Native_File_Chooser -- Makefile
#

# UNCOMMENT FOR FLTK1 BUILD
FLTKCONFIG=fltk-config

# UNCOMMENT FOR FLTK2 BUILD
FLTK2CONFIG=fltk2-config

### REMOVE:END

# Build Everything
all: FORCE
ifneq ($(FLTKCONFIG),)
	@echo "***"; echo "*** FLTK1 BUILD"; echo "***"
	( make -f Makefile-fltk1 all "FLTKCONFIG=$(FLTKCONFIG)")
endif
ifneq ($(FLTK2CONFIG),)
	@echo "***"; echo "*** FLTK2 BUILD"; echo "***"
	( make -f Makefile-fltk2 all "FLTK2CONFIG=$(FLTK2CONFIG)")
endif

# Libs
Fl_Native_File_Chooser.o: FORCE
	( make -f Makefile-fltk1 $@ "FLTKCONFIG=$(FLTKCONFIG)")

NativeFileChooser.o: FORCE
	( make -f Makefile-fltk2 $@ "FLTK2CONFIG=$(FLTK2CONFIG)")

# Clean build
clean: FORCE
# BINARIES
	-rm -f *.o                2> /dev/null
	-rm -f *.obj              2> /dev/null
	-rm -f *.exe              2> /dev/null
	-rm -f test-browser       2> /dev/null
	-rm -f test-browser-fltk2 2> /dev/null
	-rm -f simple-app         2> /dev/null
	-rm -f simple-app-fltk2   2> /dev/null
# EDITOR CRAP
	-rm -f *~     2> /dev/null
	-rm -f .*.swp 2> /dev/null
# WINDOWS CRAP
	-rm -rf ii_files           2> /dev/null
	-rm -f *.{ilk,pdb,sln,suo} 2> /dev/null
	-rm -f *.{manifest,idb}    2> /dev/null
# UNIX CRAP
	-rm -f core         2> /dev/null
	-rm -f core.*       2> /dev/null
	-rm -f .nfs*        2> /dev/null
	-rm -f .gdb_history 2> /dev/null
# MAC CRAP
	-rm -rf .DS_Store      2> /dev/null
	-rm -f ._test-browser* 2> /dev/null
	-rm -f ._simple-app*   2> /dev/null
	-rm -f *.app/Contents/MacOS/*

# Create distribution tar file (for maintainer only)
tar: clean
	if grep -q ERCODEBUG *.cxx FL/*.H ; then \
	    echo '### NO DEBUG CODE IN RELEASE'; \
	    exit 1; \
	fi
	sh .TAR_RELEASE.sh

# FORCE TARGET -- Do not remove
FORCE:
