INCLUDES= -I$(top_srcdir)/include $(LIBXML_CFLAGS) $(LIBGSL_CFLAGS)

bin_PROGRAMS = wrekavoc wrekavocd cpulim cpulimd burn wrekavalid wrekaconf

wrekavoc_SOURCES = wrekavoc.c builder.c utils.c
wrekavoc_LDFLAGS = $(LIBXML_CFLAGS) $(LIBGSL_CFLAGS)
wrekavoc_LDADD = $(LIBXML_LIBS) $(LIBGSL_LIBS)

wrekavocd_SOURCES = wrekavocd.c utils.c
wrekavocd_LDFLAGS = $(LIBXML_CFLAGS) $(LIBGSL_CFLAGS)
wrekavocd_LDADD = $(LIBXML_LIBS) $(LIBGSL_LIBS)

cpulim_SOURCES = cpulim.c

cpulimd_SOURCES = cpulimd.c

burn_SOURCES = burn.c 
burn_LDADD = $(MATH_LIBS) $(PTHREADS_LIBS)

wrekavalid_SOURCES = wrekavalid.c
wrekavalid_LDADD = $(PTHREADS_LIBS)

wrekaconf_SOURCES = wrekaconf.c

man1dir=/usr/share/man/man1

#install:
#	cp wrekavoc.1 $(man1dir)/
#uninstall:
#	rm $(man1dir)/wrekavoc.1
