#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([recdvb],[1.0.4],[tokita.hiroshi@gmail.com])
AC_CONFIG_SRCDIR([config.h.in],[src/Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# required version of gstreamer and gst-plugins-base
GST_REQUIRED=1.0.0

# required version of automake
AM_INIT_AUTOMAKE([foreign])

# enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])

# check for tools (compiler etc.)
# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O


# required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.


# error out if we can't find pkg-config
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed or set the PATH.])
])

# =======================================================================
# Check for the required version of GStreamer core (and gst-plugins-base)
#
# This will export GST_CFLAGS and GST_LIBS variables for use in Makefile.am
#
# If you need libraries from gst-plugins-base here, also add:
# for libgstaudio-1.0: gstreamer-audio-1.0 >= $GST_REQUIRED
# for libgstvideo-1.0: gstreamer-video-1.0 >= $GST_REQUIRED
# for libgsttag-1.0: gstreamer-tag-1.0 >= $GST_REQUIRED
# for libgstpbutils-1.0: gstreamer-pbutils-1.0 >= $GST_REQUIRED
# for libgstfft-1.0: gstreamer-fft-1.0 >= $GST_REQUIRED
# for libgstinterfaces-1.0: gstreamer-interfaces-1.0 >= $GST_REQUIRED
# for libgstrtp-1.0: gstreamer-rtp-1.0 >= $GST_REQUIRED
# for libgstrtsp-1.0: gstreamer-rtsp-1.0 >= $GST_REQUIRED
# etc.
# =======================================================================

PKG_CHECK_MODULES(GST, [
    gstreamer-1.0              >= $GST_REQUIRED
    gstreamer-base-1.0         >= $GST_REQUIRED
    gstreamer-controller-1.0   >= $GST_REQUIRED
  ], [
    AC_SUBST(GST_CFLAGS)
    AC_SUBST(GST_LIBS)
  ], [
  AC_MSG_ERROR([
    Can't find the following GStreamer development packages:

      gstreamer-1.0              >= $GST_REQUIRED
      gstreamer-base-1.0         >= $GST_REQUIRED
      gstreamer-controller-1.0   >= $GST_REQUIRED

    Please make sure you have the necessary GStreamer-1.0
    development headers installed.

    On debian/Ubuntu systems you will probably need to install the
    'libgstreamer1.0-dev' and 'libgstreamer-plugins-base1.0-dev' packages.

    On RPM-based systems you will probably need to install the
    'gstreamer-devel-1.0' package.
  ])
])

# check if compiler understands -Wall (if yes, add -Wall to GST_CFLAGS)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([ ], [ ])], [
  GST_CFLAGS="$GST_CFLAGS -Wall"
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])
                                                                                
# =======================================================================
# Finally, create Makefiles in all directories
# =======================================================================

# Checks for library functions.

AC_CONFIG_FILES([Makefile po/Makefile.in
                 src/Makefile])
AC_OUTPUT
