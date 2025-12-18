icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = wmudmount-lock.png wmudmount-unlock.png
BUILT_SOURCES = $(icon_DATA)
CLEANFILES = $(icon_DATA)

%.png: $(top_srcdir)/icons/scalable/%.svg
	rsvg-convert -w 48 -h 48 -f png $< > $@
