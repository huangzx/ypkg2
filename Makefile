YPKG=ypkg2
YPKGIMPORT=ypkg2-import
LIBYPK= libypk.so
OBJS= download.o util.o db.o data.o archive.o xml.o package.o 
DESTDIR=
BINDIR= $(DESTDIR)/usr/bin
LIBDIR= $(DESTDIR)/usr/lib
LANGDIR= $(DESTDIR)/usr/share/locale/zh_CN/LC_MESSAGES
DATADIR= $(DESTDIR)/var/ypkg/db
TMPDIR=/tmp

LIBS= -lcurl -lsqlite3 -larchive -lxml2 -lpthread

all: $(LIBYPK) $(YPKG) $(YPKGIMPORT)

$(YPKG): ypkg.o 
	cc -g -o $(YPKG)  ypkg.o  -L. -lypk

ypkg.o: ypkg.c
	cc -c -g ypkg.c -o ypkg.o


$(YPKGIMPORT): ypkg-import.o $(OBJS)	
	cc -g -o $(YPKGIMPORT)  ypkg-import.o -L. -lypk

ypkg-import.o: ypkg-import.c
	cc -c -g ypkg-import.c -o ypkg-import.o


$(LIBYPK): $(OBJS)
	cc -g -shared -fPIC -o libypk.so $(OBJS) $(LIBS)

download.o: download.c
	cc -c -g download.c -o download.o

util.o: util.c
	cc -c -g util.c -o util.o

db.o: db.c
	cc -c -g db.c -o db.o

data.o: data.c
	cc -c -g data.c -o data.o

archive.o: archive.c
	cc -c -g archive.c -o archive.o

xml.o: xml.c
	cc -c -g xml.c -o xml.o

package.o: package.c
	cc -c -g package.c -o package.o

install: all
	mkdir -p $(BINDIR) $(LIBDIR) $(LANGDIR) $(DATADIR) $(TMPDIR)
	cp $(LIBYPK) $(LIBDIR)
	cp $(YPKG) $(BINDIR)
	cp $(YPKGIMPORT) $(BINDIR) 
	cp data/db_create.sql $(TMPDIR)
	sqlite3 $(DATADIR)/package.db ".read $(TMPDIR)/db_create.sql"
	#cp po/zh_CN.mo $(LANGDIR)/ypkg.mo
	$(BINDIR)/$(YPKGIMPORT)

clean:
	rm -f $(OBJS) ypkg.o ypkg-import.o $(LIBYPK) $(YPKG) $(YPKGIMPORT) $(TMPDIR)/db_create.sql

remove:
	rm -f $(BINDIR)/$(YPKG) 
	rm -f $(BINDIR)/$(YPKGIMPORT)
	rm -f $(LIBDIR)/$(LIBYPK) 
	rm -f $(DATADIR)/package.db
	#rm -f $(LANGDIR)/ypkg.mo
