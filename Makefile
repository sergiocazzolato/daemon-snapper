all:
	gcc src/exampled.c -o exampled
	gcc src/exampleauxd.c -o exampleauxd

install:
	mkdir -p $(DESTDIR)/bin
	cp exampled $(DESTDIR)/bin/exampled
	cp exampleauxd $(DESTDIR)/bin/exampleauxd
