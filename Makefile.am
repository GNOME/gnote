ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = 

INTLTOOL_FILES = intltool-extract.in \
                 intltool-merge.in \
                 intltool-update.in

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 po/.intltool-merge-cache

if HAVE_X11_SUPPORT
LIBTOMBOYDIR = libtomboy
endif

SUBDIRS = data $(LIBTOMBOYDIR) src po help

DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper --with-cxx11-support --with-x11-support

# Ignore scrollkeeper issues for now.  @#*$& scrollkeeper (from Evince)

