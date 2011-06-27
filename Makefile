#########################################################################
# vim: ts=8 sw=8
#########################################################################
# Author:   reynolds (Tommy Reynolds)
# Filename: Makefile
# Date:     2007-01-03 19:07:43
#########################################################################
# Note that we use '::' rules to allow multiple rule sets for the same
# target.  Read that as "modularity exemplarized".
#########################################################################

PREFIX	:=/opt/backbencher
BINDIR	=${PREFIX}/bin

TARGETS	=all clean distclean clobber check install uninstall tags
TARGET	=all

SUBDIRS	=

.PHONY:	${TARGETS} ${SUBDIRS}

CC	=gcc -mtune=native -std=gnu99
INC	=-I.
CFLAGS	=-Os -Wall -Wextra -Werror -pedantic -g
CFLAGS	+=`getconf LFS_CFLAGS`
CFLAGS	+=${DEFS}
CFLAGS	+=${INC}
LDFLAGS	=-g
LDFLAGS	+=`getconf LFS_LDFLAGS`
LDLIBS	=

DATAFILE=/usr/tdrs/data/archive/HDF/${USER}/datafile

all::	backbencher

${TARGETS}::

clean::
	${RM} a.out *.o core.* lint tags
	${RM} ${DATAFILE}

distclean clobber:: clean
	${RM} backbencher

CHUNK	=0
ARGS	=-n4096000000 -c ${CHUNK}

check::	backbencher
	# Let write role testing create the file
	${RM} ${DATAFILE}
	/usr/bin/time -p ./backbencher ${ARGS} ${DATAFILE}
	ls -lh ${DATAFILE}
	${RM} ${DATAFILE}
	/usr/bin/time -p ./backbencher -m ${ARGS} ${DATAFILE}
	ls -lh ${DATAFILE}
	# Do read role testing using leftover data file
	/usr/bin/time -p ./backbencher -r ${ARGS} ${DATAFILE}
	/usr/bin/time -p ./backbencher -rm ${ARGS} ${DATAFILE}

install:: backbencher
	install -d ${BINDIR}
	install -c -s backbencher ${BINDIR}/

uninstall::
	${RM} ${BINDIR}/backbencher

tags::
	ctags -R .

# ${TARGETS}::
# 	${MAKE} TARGET=$@ ${SUBDIRS}

# ${SUBDIRS}::
# 	${MAKE} -C $@ ${TARGET}
