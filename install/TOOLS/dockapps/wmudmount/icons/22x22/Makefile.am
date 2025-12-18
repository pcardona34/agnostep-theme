icondir = $(datadir)/icons/hicolor/22x22/apps
icon_DATA = wmudmount-lock.png wmudmount-unlock.png
BUILT_SOURCES = $(icon_DATA)
CLEANFILES = $(icon_DATA)

%.png: $(top_srcdir)/icons/scalable/%.svg
	rsvg-convert -w 22 -h 22 -f png $< > $@
