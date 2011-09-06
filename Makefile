YPKG=ypkg2
YPKGIMPORT=ypkg2-import
OBJS= download.o util.o db.o archive.o xml.o package.o 
DESTDIR=
BINDIR= $(DESTDIR)/usr/bin
LANGDIR= $(DESTDIR)/usr/share/locale/zh_CN/LC_MESSAGES
DATADIR= $(DESTDIR)/var/ypkg/db
TMPDIR=/tmp

LIBS= -lcurl -lsqlite3 -larchive -lxml2 -lpthread

all: $(YPKG) $(YPKGIMPORT)

$(YPKG): ypkg.o $(OBJS)	
	cc -g -o $(YPKG)  ypkg.o $(OBJS) $(LIBS)

$(YPKGIMPORT): ypkg-import.o $(OBJS)	
	cc -g -o $(YPKGIMPORT)  ypkg-import.o $(OBJS) $(LIBS)

ypkg.o: ypkg.c
	cc -c -g ypkg.c -o ypkg.o

ypkg-import.o: ypkg-import.c
	cc -c -g ypkg-import.c -o ypkg-import.o

OBJS= download.o util.o db.o archive.o xml.o package.o 

download.o: download.c
	cc -c -g download.c -o download.o

util.o: util.c
	cc -c -g util.c -o util.o

db.o: db.c
	cc -c -g db.c -o db.o

archive.o: archive.c
	cc -c -g archive.c -o archive.o

xml.o: xml.c
	cc -c -g xml.c -o xml.o

package.o: package.c
	cc -c -g package.c -o package.o

install: all
	mkdir -p $(BINDIR) $(LANGDIR) $(DATADIR) $(TMPDIR)
	cp $(YPKG) $(BINDIR)
	cp $(YPKGIMPORT) $(BINDIR) 
	cp data/db_create.sql $(TMPDIR)
	sqlite3 $(DATADIR)/package.db ".read $(TMPDIR)/db_create.sql"
	#cp po/zh_CN.mo $(LANGDIR)/ypkg.mo

clean:
	rm -f *.o $(YPKG) $(YPKGIMPORT) $(TMPDIR)/db_create.sql

remove:
	rm -f $(BINDIR)/$(YPKG) 
	rm -f $(BINDIR)/$(YPKGIMPORT)
	rm -f $(DATADIR)/db/package.db
	#rm -f $(LANGDIR)/ypkg.mo
