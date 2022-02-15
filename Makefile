## Configuration
DESTDIR    =
PREFIX     =/usr/local
AR         =ar
CC         =gcc
CFLAGS     =-Wall -g
CPPFLAGS   =
LIBS       =\
    "-l:libmdb.a"  \
    "-l:libgdbm.a" \
    "-l:libuuid.a"
## Sources and targets
PROGRAMS   =mcontact
LIBRARIES  =libmcontact.a
HEADERS    =mcontact.h
MARKDOWNS  =README.md mcontact.3.md
MANPAGES_3 =
SOURCES    =main.c mcontact.c
## AUXILIARY
CFLAGS_ALL =$(LDFLAGS) $(CFLAGS) $(CPPFLAGS)

## STANDARD TARGETS
all: $(PROGRAMS) $(LIBRARIES)
help:
	@echo "all     : Build everything."
	@echo "clean   : Clean files."
	@echo "install : Install all produced files."
install: all
	install -d                  $(DESTDIR)$(PREFIX)/bin
	install -m755 $(PROGRAMS)   $(DESTDIR)$(PREFIX)/bin
	install -d                  $(DESTDIR)$(PREFIX)/include
	install -m644 $(HEADERS)    $(DESTDIR)$(PREFIX)/include
	install -d                  $(DESTDIR)$(PREFIX)/lib
	install -m644 $(LIBRARIES)  $(DESTDIR)$(PREFIX)/lib
clean:
	rm -f $(PROGRAMS) $(LIBRARIES)
ssnip:
	ssnip LICENSE $(MARKDOWNS) $(HEADERS) $(SOURCES) $(MANPAGES_3)

libmcontact.a : mcontact.c $(HEADERS)
	$(CC) -o mcontact.o -c mcontact.c $(CFLAGS_ALL)
	$(AR) -crs $@ mcontact.o
	rm -f mcontact.o
mcontact: main.c libmcontact.a $(HEADERS)
	$(CC) -o $@ main.c libmcontact.a $(CFLAGS_ALL) $(LIBS)
