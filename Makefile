SERVICE := ptth
DESTDIR ?= dist_root
SERVICEDIR ?= /srv/$(SERVICE)

.PHONY: build install

build:
	$(MAKE) -C src

install: build
	install -d $(DESTDIR)$(SERVICEDIR)
	install -d $(DESTDIR)$(SERVICEDIR)/bin
	install src/sister $(DESTDIR)$(SERVICEDIR)/bin/ptth
	install -d $(DESTDIR)$(SERVICEDIR)/data
	install -d $(DESTDIR)/etc/systemd/system
	install -m 644 src/ptth.service $(DESTDIR)/etc/systemd/system/
