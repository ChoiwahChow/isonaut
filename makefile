# makefile for nauty 2.8
# @configure_input@

SHELL=/bin/bash
#CC=gcc
# CFLAGS= -O4  -mpopcnt -march=native
CC=g++
CFLAGS= -std=c++11 -O3  -mpopcnt -march=native
SAFECFLAGS= -O3
LDFLAGS=
LOK=1         # 0 if no 64-bit integers


# Trailing spaces in the following will cause problems

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
pkgconfigexecdir=${prefix}/libdata/pkgconfig

INSTALL=@INSTALL@
INSTALL_DATA=@INSTALL_DATA@
MKDIR_P=@MKDIR_P@

SMALL=-DMAXN=WORDSIZE
S1=-DMAXN=WORDSIZE -DWORDSIZE=16
W1=-DMAXN=WORDSIZE -DWORDSIZE=32
L1=-DMAXN=WORDSIZE -DWORDSIZE=64
S=-DWORDSIZE=16
W=-DWORDSIZE=32
L=-DWORDSIZE=64

CCOBJ=${CC} -c ${CFLAGS} -o $@
GTOOLSH=gtools.h nauty.h naututil.h nausparse.h naurng.h gutils.h \
  naugroup.h nautinv.h schreier.h nautycliquer.h traces.h \
  naugstrings.h planarity.h quarticirred28.h
GTOOLS=copyg listg labelg dretog amtog geng complg showg NRswitchg \
  biplabg addedgeg deledgeg countg pickg genrang newedgeg catg genbg \
  directg gentreeg genquarticg underlyingg assembleg gengL addptg \
  ranlabg multig planarg gentourng linegraphg watercluster2 dretodot \
  subdivideg vcolg delptg cubhamg twohamg hamheuristic converseg \
  genposetg nbrhoodg genspecialg edgetransg genbgL dreadnaut \
  ancestorg productg dimacs2g @shortg_or_null@
GLIBS=nauty.a 
PCFILE=nauty.pc

# @edit_msg@

all : isonaut ;


isonaut : nauty.h nausparse.h nauty_utils.h test_iso_graph.h isofilter.h CLI11.hpp \
          nauty_utils.c test_iso_graph.c isofilter.c isofilter_main.c \
          ${GLIBS}
	${CC} -o isonaut ${CFLAGS} nauty_utils.c test_iso_graph.c isofilter.c isofilter_main.c ${GLIBS} ${LDFLAGS}


# @edit_msg@
