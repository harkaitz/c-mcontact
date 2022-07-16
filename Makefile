## Configuration
DESTDIR      =
PREFIX       =/usr/local
AR           =ar
CC           =gcc
CFLAGS       =-Wall -g
PROGRAMS     =mcontact$(EXE)
LIBRARIES    =libmcontact.a
HEADERS      =mcontact.h
CFLAGS_ALL   =$(LDFLAGS) $(CFLAGS) $(CPPFLAGS)
LIBS         =\
    "-l:libmdb.a"     \
    "-l:libhiredis.a" \
    "-l:libuuid.a"

## 
all: $(PROGRAMS) $(LIBRARIES)
install: all
	install -d                  $(DESTDIR)$(PREFIX)/bin
	install -m755 $(PROGRAMS)   $(DESTDIR)$(PREFIX)/bin
	install -d                  $(DESTDIR)$(PREFIX)/include
	install -m644 $(HEADERS)    $(DESTDIR)$(PREFIX)/include
	install -d                  $(DESTDIR)$(PREFIX)/lib
	install -m644 $(LIBRARIES)  $(DESTDIR)$(PREFIX)/lib
clean:
	rm -f $(PROGRAMS) $(LIBRARIES)

##
libmcontact.a : mcontact.c $(HEADERS)
	$(CC) -o mcontact.o -c mcontact.c $(CFLAGS_ALL)
	$(AR) -crs $@ mcontact.o
	rm -f mcontact.o
mcontact$(EXE): main.c libmcontact.a $(HEADERS)
	$(CC) -o $@ main.c libmcontact.a $(CFLAGS_ALL) $(LIBS)


## -- manpages --
ifneq ($(PREFIX),)
MAN_3=./mcontact.3 
install: install-man3
install-man3: $(MAN_3)
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man3
	cp $(MAN_3) $(DESTDIR)$(PREFIX)/share/man/man3
endif
## -- manpages --
## -- license --
ifneq ($(PREFIX),)
install: install-license
install-license: LICENSE
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/c-mcontact
	cp LICENSE $(DESTDIR)$(PREFIX)/share/doc/c-mcontact
endif
## -- license --
## -- gettext --
ifneq ($(PREFIX),)
install: install-po
install-po:
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale/es/LC_MESSAGES
	cp locales/es/LC_MESSAGES/c-mcontact.mo $(DESTDIR)$(PREFIX)/share/locale/es/LC_MESSAGES
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale/eu/LC_MESSAGES
	cp locales/eu/LC_MESSAGES/c-mcontact.mo $(DESTDIR)$(PREFIX)/share/locale/eu/LC_MESSAGES
endif
## -- gettext --
