icondir = $(datadir)/icons/hicolor/16x16/apps
icon_DATA = wmudmount-lock.png wmudmount-unlock.png
BUILT_SOURCES = $(icon_DATA)
CLEANFILES = $(icon_DATA)

%.png: $(top_srcdir)/icons/scalable/%.svg
	rsvg-convert -w 16 -h 16 -f png $< > $@
