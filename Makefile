VERSION=0.1.2
YPKG=ypkg2
YGET=yget2
YPKGIMPORT=ypkg2-import
YPKGGENCONTROL=ypkg2-gencontrol
LIBYPK= libypk.so
YPACKAGEH=ypackage.h
OBJS= download.o util.o db.o data.o archive.o xml.o preg.o ypackage.o sha1.o
DEBUG= 
W= -Wall
O= -O2
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
	cc $(W) $(DEBUG) -o $(YPKG)  ypkg.o  -L. -lypk

ypkg.o: ypkg.c
	cc $(W) $(DEBUG) $(O) -c ypkg.c -o ypkg.o

$(YGET): yget.o 
	cc $(W) $(DEBUG) -o $(YGET) yget.o  -L. -lypk

yget.o: yget.c
	cc $(W) $(DEBUG)  $(O) -c yget.c -o yget.o

$(YPKGIMPORT): ypkg-import.o 
	cc $(W) $(DEBUG) -o $(YPKGIMPORT)  ypkg-import.o -L. -lypk

ypkg-import.o: ypkg-import.c
	cc $(W) $(DEBUG)  $(O) -c ypkg-import.c -o ypkg-import.o


$(LIBYPK): $(OBJS)
	cc $(W) $(DEBUG) -shared -fPIC -o libypk.so $(OBJS) $(LIBS)

$(STATIC_LIB):$(OBJS)
	ar -r $(STATIC_LIB) $(OBJS)
	ranlib $(STATIC_LIB)

download.o: download.c
	cc -c $(W)$(DEBUG)  $(O) download.c -o download.o

util.o: util.c
	cc -c $(W)$(DEBUG)  $(O) util.c -o util.o

db.o: db.c
	cc -c $(W)$(DEBUG)  $(O) db.c -o db.o

data.o: data.c
	cc -c $(W)$(DEBUG)  $(O) data.c -o data.o

archive.o: archive.c
	cc -c $(W)$(DEBUG)  $(O) -D_FILE_OFFSET_BITS=64 archive.c -o archive.o

xml.o: xml.c
	cc -c $(W)$(DEBUG)  $(O) xml.c -o xml.o

preg.o: preg.c
	cc -c $(W)$(DEBUG)  $(O) preg.c -o preg.o

sha1.o: sha1.c
	cc -c $(W)$(DEBUG)  $(O) sha1.c -o sha1.o

ypackage.o: ypackage.c
	cc -c $(W)$(DEBUG)  $(O) ypackage.c -o ypackage.o

install: all
	mkdir -p $(BINDIR) $(LIBDIR) $(INCDIR) $(LANGDIR) $(DBDIR) $(DATADIR) 
	cp $(LIBYPK) $(LIBDIR)/$(LIBYPK).$(VERSION)
	cp $(YPACKAGEH) $(INCDIR)
	cp $(YPKG) $(BINDIR)
	cp $(YGET) $(BINDIR)
	cp $(YPKGIMPORT) $(BINDIR) 
	cp $(YPKGGENCONTROL) $(BINDIR) 
	#cp po/zh_CN.mo $(LANGDIR)/ypkg.mo
	cp data/db_create.sql $(DATADIR)
	#sqlite3 $(DBDIR)/package.db ".read $(DATADIR)/db_create.sql"
	cd  $(LIBDIR) && ln -s $(LIBYPK).$(VERSION) ./$(LIBYPK)
	#$(BINDIR)/$(YPKGIMPORT)
	

clean:
	rm -f $(OBJS) ypkg.o yget.o ypkg-import.o $(LIBYPK) $(YPKG) $(YGET) $(YPKGIMPORT) $(STATIC_LIB)

remove:
	rm -f $(BINDIR)/$(YPKG) 
	rm -f $(BINDIR)/$(YGET) 
	rm -f $(BINDIR)/$(YPKGIMPORT)
	rm -f $(BINDIR)/$(YPKGGENCONTROL)
	rm -f $(LIBDIR)/$(LIBYPK).$(VERSION) 
	rm -f $(LIBDIR)/$(LIBYPK)
	rm -f $(INCDIR)/$(YPACKAGEH) 
	rm -f $(DATADIR)/db_create.sql
	#rm -f $(DBDIR)/package.db
	#rm -f $(LANGDIR)/ypkg.mo
