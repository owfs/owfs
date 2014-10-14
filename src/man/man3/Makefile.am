## man sources
MANFILES = \
    DS1821.man DS1822.man DS1825.man DS18B20.man DS18S20.man DS1921.man \
    DS1963L.man DS1963S.man DS1977.man DS1991.man DS1992.man DS1993.man \
    DS1995.man DS1996.man DS2401.man DS2404.man DS2405.man DS2406.man \
    DS2408.man DS2409.man DS2413.man DS2415.man DS2423.man DS2430A.man \
    DS2431.man DS2433.man DS2436.man DS2437.man DS2438.man DS2450.man \
    DS2502.man DS2505.man DS2506.man DS2720.man DS2740.man DS2751.man \
    DS2755.man DS2760.man DS2770.man DS2780.man DS2781.man DS2890.man \
    DS28E04.man DS28EA00.man DS28EC20.man EDS.man EEEF.man LCD.man mAM001.man \
    mCM001.man mDI001.man mRS001.man owperl.man
## .so includes
SOFILES = \
    addressing.3so description.3so seealso.3so standard.3so standard_mini.3so \
    temperature_errata.3so temperature_threshold.3so temperatures_mini.3so
## man files that need no preprocessing
dist_man3_MANS = \
    DS1427.3 DS1904.3 DS1920.3 DS1971.3 DS1982.3 DS1982U.3 DS1985.3 DS1985U.3 \
    DS1986.3 DS1986U.3 DS1990A.3 DS1994.3 DS2404S.3 DS2407.3 DS2411.3 \
    DS2417.3 DS2502-E48.3 DS2502-UNW.3 DS2505-UNW.3 DS2506-UNW.3 DS2756.3 \
    DS2761.3 DS2762.3 DS2788.3 DS2804.3 EDS0064.3 EDS0065.3 EDS0066.3 \
    EDS0067.3 EDS0068.3 EDS0070.3 EDS0071.3 EDS0072.3 EDS0080.3 EDS0082.3 \
    EDS0083.3 EDS0085.3 EDS0090.3 MAX31820.3 MAX31826.3 MAX31850.3 MAX31851.3 \
    OWNet.3 Thermachron.3 UVI.3
## file to include in distribution
EXTRA_DIST = $(SOFILES) $(MANFILES)


if SOELIM

man3_MANS = $(addsuffix .3,$(basename $(MANFILES)))
CLEANFILES = $(man3_MANS)

# preproc man pages via soelim
$(man3_MANS): $(MANFILES) $(SOFILES)
%.3 :: %.man
	$(SOELIM) -r -I $(srcdir)/.. $< > $@

else !SOELIM

man3_MANS = $(MANFILES) $(SOFILES)

endif !SOELIM
