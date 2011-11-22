VERSION=0.0.1
YPKG=ypkg2
YGET=yget2
YPKGIMPORT=ypkg2-import
LIBYPK= libypk.so
YPACKAGEH=ypackage.h
OBJS= download.o util.o db.o data.o archive.o xml.o preg.o ypackage.o 
DEBUG= 
DESTDIR=
BINDIR= $(DESTDIR)/usr/bin
LIBDIR= $(DESTDIR)/usr/lib
INCDIR= $(DESTDIR)/usr/include
LANGDIR= $(DESTDIR)/usr/share/locale/zh_CN/LC_MESSAGES
DBDIR= $(DESTDIR)/var/ypkg/db
DATADIR= $(DESTDIR)/usr/share/ypkg
STATIC_LIB=libypk.a

LIBS= -lcurl -lsqlite3 -larchive -lxml2 -lpthread -lpcre

all: $(LIBYPK) $(YPKG) $(YGET) $(YPKGIMPORT) $(STATIC_LIB)

$(YPKG): ypkg.o 
	cc  $(DEBUG) -g -o $(YPKG)  ypkg.o  -L. -lypk

ypkg.o: ypkg.c
	cc  $(DEBUG) -c ypkg.c -o ypkg.o

$(YGET): yget.o 
	cc  $(DEBUG) -o $(YGET) yget.o  -L. -lypk

yget.o: yget.c
	cc  $(DEBUG) -c yget.c -o yget.o

$(YPKGIMPORT): ypkg-import.o 
	cc  $(DEBUG) -o $(YPKGIMPORT)  ypkg-import.o -L. -lypk

ypkg-import.o: ypkg-import.c
	cc  $(DEBUG) -c ypkg-import.c -o ypkg-import.o


$(LIBYPK): $(OBJS)
	cc  $(DEBUG) -shared -fPIC -o libypk.so $(OBJS) $(LIBS)

$(STATIC_LIB):$(OBJS)
	ar -r $(STATIC_LIB) $(OBJS)
	ranlib $(STATIC_LIB)

download.o: download.c
	cc -c $(DEBUG) download.c -o download.o

util.o: util.c
	cc -c $(DEBUG) util.c -o util.o

db.o: db.c
	cc -c $(DEBUG) db.c -o db.o

data.o: data.c
	cc -c $(DEBUG) data.c -o data.o

archive.o: archive.c
	cc -c $(DEBUG) archive.c -o archive.o

xml.o: xml.c
	cc -c $(DEBUG) xml.c -o xml.o

preg.o: preg.c
	cc -c $(DEBUG) preg.c -o preg.o

ypackage.o: ypackage.c
	cc -c $(DEBUG) ypackage.c -o ypackage.o

install: all
	mkdir -p $(BINDIR) $(LIBDIR) $(INCDIR) $(LANGDIR) $(DBDIR) $(DATADIR) 
	cp $(LIBYPK) $(LIBDIR)/$(LIBYPK).$(VERSION)
	ln -s $(LIBDIR)/$(LIBYPK).$(VERSION) $(LIBDIR)/$(LIBYPK)
	cp $(YPACKAGEH) $(INCDIR)
	cp $(YPKG) $(BINDIR)
	cp $(YGET) $(BINDIR)
	cp $(YPKGIMPORT) $(BINDIR) 
	cp data/db_create.sql $(DATADIR)
	#sqlite3 $(DBDIR)/package.db ".read $(DATADIR)/db_create.sql"
	#$(BINDIR)/$(YPKGIMPORT)
	#cp po/zh_CN.mo $(LANGDIR)/ypkg.mo

clean:
	rm -f $(OBJS) ypkg.o yget.o ypkg-import.o $(LIBYPK) $(YPKG) $(YGET) $(YPKGIMPORT) 

remove:
	rm -f $(BINDIR)/$(YPKG) 
	rm -f $(BINDIR)/$(YGET) 
	rm -f $(BINDIR)/$(YPKGIMPORT)
	rm -f $(LIBDIR)/$(LIBYPK).$(VERSION) 
	rm -f $(LIBDIR)/$(LIBYPK)
	rm -f $(INCDIR)/$(YPACKAGEH) 
	rm -f $(DATADIR)/db_create.sql
	#rm -f $(DBDIR)/package.db
	#rm -f $(LANGDIR)/ypkg.mo
