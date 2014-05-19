# name of your binary
bin_PROGRAMS = recdvb

# list of source files
# the prefix is the name of the binary
recdvb_SOURCES = recdvb.c configs.c

# our CFLAGS and LDFLAGS used for compiling and linking
# make sure you prefix these with the name of your binary
recdvb_CFLAGS = $(CFLAGS) $(GST_CFLAGS) --std=c11 -Wall -DSYSCONFDIR=\"@sysconfdir@\" -DCONFFILENAME=\"recdvbrc\"
recdvb_LDFLAGS = $(LDFLAGS) $(GST_LIBS)
man1_MANS = recdvb.1

sysconf_DATA = recdvbrc

recdvb.1: recdvb.1.xml
	xsltproc --nonet \
	       	--param make.year.ranges 1 \
	       	--param make.single.year.ranges 1 \
	       	--param man.charmap.use.subset 0 \
	       	http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl \
	       	recdvb.1.xml

