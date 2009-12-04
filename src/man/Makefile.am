## $Id$

mandir = @mandir@

man_MANS = owfs.1 \
           owhttpd.1 \
           owserver.1 \
           owftpd.1 \
           owcapi.1 libowcapi.1 \
           ownet.1 ownetapi.1 libownet.1 ownetlib.1 \
           owshell.1 owdir.1 owread.1 owwrite.1 owget.1 owpresent.1 \
           owtap.1 owmon.1 \
           DS1427.3 \
           DS1821.3 DS1822.3 DS1825.3 DS18B20.3 DS18S20.3 DS28EA00.3 \
           DS1904.3 DS1920.3 DS1921.3 DS1963L.3 DS1963S.3 DS1977.3 \
           DS1982.3 DS1982U.3 DS1985.3 DS1985U.3 DS1986.3 DS1986U.3 \
           DS1990A.3 DS1991.3 DS1992.3 DS1993.3 DS1994.3 DS1995.3 DS1996.3 \
           DS2401.3 DS2404.3 DS2404S.3 DS2405.3 DS2406.3 DS2407.3 DS2408.3 DS2409.3 \
           DS2411.3 DS2413.3 DS2415.3 DS2417.3 \
           DS2423.3 \
           DS2430A.3 DS2431.3 DS2433.3 DS2436.3 DS2437.3 DS2438.3 \
           DS2450.3 \
           DS2502.3 DS2502-E48.3 DS2502-UNW.3 DS2505.3 DS2505-UNW.3 DS2506.3 DS2506-UNW.3 \
           DS2720.3 DS2740.3 DS2751.3 DS2755.3 DS2756.3 DS2760.3 DS2761.3 DS2762.3 DS2770.3 \
           DS2780.3 DS2781.3 DS2788.3 \
           DS2804.3 DS28E04.3 DS28EA00.3 DS28EC20.3 \
           DS2890.3 \
	   EEEF.3 UVI.3 \
           Thermachron.3 \
           LCD.3 \
           OWNet.3 \
           owperl.3 \
           owtcl.3 \
           owfs.5 owfs.conf.5 \
           owtcl.n

EXTRA_DIST = ${man_MANS}

install-data-am: install-man install-so-man

install-so-man:
	for i in $(man_MANS); do \
	    case $$i in \
		*.1*) mandirr=$(man1dir) ;; \
		*.3*) mandirr=$(man3dir) ;; \
		*.5*) mandirr=$(man5dir) ;; \
		*.n*) mandirr=$(manndir) ;; \
	    esac; \
	    i=$$mandirr/$$i; \
	    list="$$list $$i"; \
	done; \
	for manfile in $$list; do \
	    for i in `sed -n '2,$$ s/^.so //p' $(DESTDIR)/$$manfile`; do \
			sed -i "/$$i/r $(srcdir)/$$i" $(DESTDIR)/$$manfile; \
			sed -i "/$$i/d" $(DESTDIR)/$$manfile; \
	    done; \
		sed -i '/^.\\\"/d' $(DESTDIR)/$$manfile; \
	done

clean-generic:

	@RM@ -f *~ .*~

