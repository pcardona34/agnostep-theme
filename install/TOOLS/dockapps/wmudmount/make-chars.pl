#!/usr/bin/perl -w
$copyright = <<EOC ;
/* wmudmount
 * Copyright © 2010-2014  Brad Jorsch <anomie\@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
EOC

# This script reads the fixed-width character data from chars-mask.xpm and
# outputs the C file chars.c with the necessary datastructures and functions
# to use it as a variable width font in various colors.

# As written, chars-mask.xpm must contain 95 characters representing ASCII
# 33-126 and U+FFFD.

# Character cell size
$ww=5;
$hh=5;

open IN, "<chars-mask.xpm" or die "Could not open chars.xpm\n";
open OUT, ">chars.c" or die "Could not open chars.c\n";

print OUT <<EOH ;
$copyright
#include "config.h"
#include "chars.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

EOH

# Parse the XPM
$head='';
$name='';
$colorbuf='';
$colorbuflen=0;
$colors='';
@mask=();
$black=undef;
@lines=();

while(<IN>){
    $head.=$_;
    if(/^static char \* ([a-zA-Z0-9_]+_xpm)\[\] = {/){
        $name=$1;
        last;
    }
}

$_=<IN>;
die "Invalid size spec line\n" unless /^"(\d+) (\d+) (\d+) (\d+)"/;
($w,$h,$c,$d)=(+$1,+$2,+$3,+$4);
die "Invalid width\n" if $w != 95*$ww*$d;
die "Invalid height\n" if $h != $hh;

$comment=' /* Point colors to writable memory declared above */';
for($i=0; $i<$c; $i++){
    $_=<IN>;
    die "Invalid color line for color $i\n" unless /^"(.{$d})\tc (#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2}))",$/i;
    $colors.="colorbuf+$colorbuflen,$comment\n";
    $comment='';
    $colorbuf.="$1\\tc #xxxxxx\\0";
    die "Mask pixmap must be RGB greyscale\n" unless($3 eq $4 && $4 eq $5);
    push @mask, hex($3);
    $colorbuflen+=11+$d;
    if($2 eq '#000000'){
        die "Multiple blacks?\n" if defined($black);
        $black=$1;
    }
}

$cw=$w*$d;
for($i=0; $i<$h-1; $i++){
    $_=<IN>;
    die "Invalid image line $i\n" unless /^"(.{$cw})",$/;
    $lines[$i]=$1;
}
$_=<IN>;
die "Invalid image line $i\n" unless /^"(.{$cw})"};$/;
$lines[$i]=$1;
close IN;

# Trim whitespace from the edges of each character
$newwidth=0;
@widths=();
for($i=$w-$ww*$d; $i>=0; $i-=$ww*$d){
    @ll=();
    for($j=0; $j<$h; $j++){
        $ll[$j]=substr($lines[$j],$i,$ww*$d);
    }
    S: for($s=0; $s<length($ll[0]); $s+=$d){
        for($j=0; $j<$h; $j++){
            last S if substr($ll[$j],$s,$d) ne $black;
        }
    }
    E: for($e=$ww; $e>0; $e-=$d){
        for($j=0; $j<$h; $j++){
            last E if substr($ll[$j],$e-$d,$d) ne $black;
        }
    }
    $widths[$i/$ww/$d]=($e-$s)/$d;
    $newwidth+=($e-$s)/$d;
    for($j=0; $j<$h; $j++){
        substr($lines[$j],$i,$ww*$d)=substr($ll[$j],$s,$e-$s);
    }
}

# Output C file
print OUT "/* Normally, the colors in an xpm would be stored as string literals, which\n";
print OUT " * may be stored in read-only memory. In order to change the colors at runtime,\n";
print OUT " * we need to store them in writable memory instead. We pack them into one big\n";
print OUT " * buffer here to save a few bytes and make the color-setting loop simpler. */\n";
$colorbuf=~s/\\0$//;
print OUT "static char colorbuf[$colorbuflen] = \"$colorbuf\";\n";

print OUT "\n";
print OUT "/* These are the mask values for each color. */\n";
print OUT "static guint8 mask[] = {".join(',',@mask)."};\n";

print OUT "\n";
print OUT "/* This xpm contains variable-width graphics for ASCII printablecharacters\n";
print OUT " * plus a \"replacement\" character vaguely reminiscent of U+FFFD. */\n";
print OUT $head;
printf OUT qq("%d %d %d %d",\n), $newwidth+2,$h,$c,$d; # +2 for ' '
print OUT $colors;
for($i=0; $i<$h-1; $i++){
    printf OUT qq("%s",\n), $black.$black.$lines[$i]; # note added pixels for ' '
}
printf OUT qq("%s"};\n), $black.$black.$lines[$i]; # note added pixels for ' '

print OUT "\n";
print OUT "/* Width of each character. */\n";
print OUT "static int widths[] = {2,".join(',',@widths)."};\n"; # note ' '

print OUT "\n";
print OUT "/* Starting X position of each character. */\n";
print OUT "static int starts[] = {0"; # note ' '
$s=2;
for($i=0; $i<@widths; $i++){
    printf OUT ",%d", $s;
    $s+=$widths[$i];
}
print OUT "};\n";

$cbofs=$d+4;
$cbmult=$d+11;

print OUT <<EOF ;

/* Height of all characters. */
const int char_height = $hh;

static GdkPixbuf **charpix = NULL;

gboolean init_chars(int numcolors, ...) {
    gboolean ret = TRUE;
    va_list ap;

    charpix = g_malloc(numcolors * sizeof(GdkPixbuf *));
    va_start(ap, numcolors);
    for (int i=0; i<numcolors; i++) {
        /* Standard color mixing algorithm. We just sprintf the RGB spec into
         * the right positions in the color buffer. */
        int r1 = va_arg(ap,int), g1 = va_arg(ap,int), b1 = va_arg(ap,int);
        int r2 = va_arg(ap,int) - r1, g2 = va_arg(ap,int) - g1, b2 = va_arg(ap,int) - b1;
        for (size_t j = 0, o = $cbofs; j < sizeof(mask); j++, o += $cbmult) {
            sprintf(colorbuf + o, "%02x%02x%02x",
                (guint8)(r1 + r2 * mask[j] / 255),
                (guint8)(g1 + g2 * mask[j] / 255),
                (guint8)(b1 + b2 * mask[j] / 255)
            );
        }
        charpix[i] = gdk_pixbuf_new_from_xpm_data((const char **)$name);
        if(charpix[i] == NULL) ret = FALSE;
    }
    va_end(ap);
    return ret;
}

gboolean chars_change_color(int color, int bg_r, int bg_g, int bg_b, int fg_r, int fg_g, int fg_b){
    g_object_unref(charpix[color]);
    int r2 = fg_r-bg_r, g2 = fg_g-bg_g, b2 = fg_b-bg_b;
    for (size_t j = 0, o = $cbofs; j < sizeof(mask); j++, o += $cbmult) {
        sprintf(colorbuf + o, "%02x%02x%02x",
            (guint8)(bg_r + r2 * mask[j] / 255),
            (guint8)(bg_g + g2 * mask[j] / 255),
            (guint8)(bg_b + b2 * mask[j] / 255)
        );
    }
    charpix[color] = gdk_pixbuf_new_from_xpm_data((const char **)$name);
    return charpix[color] != NULL;
}

int draw_char(cairo_t *cr, char c, int x, int y, int color) {
    if (c < 0x20 || c > 0x7e) {
        c=0x7f;
    }
    c -= 0x20;
    int i = c;
    int w = widths[i];

    cairo_save(cr);
    cairo_rectangle(cr, x, y, w, $hh);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, charpix[color], x - starts[i], y);
    cairo_paint(cr);
    cairo_restore(cr);
    return w;
}

int draw_string(cairo_t *cr, const char *s, int x, int y, int color) {
    if (!*s) {
        return 0;
    }
    int w = 0;
    for (; *s; s++) {
        w += draw_char(cr, *s, x+w, y, color) + 1;
    }
    return w-1;
}

int measure_string(const char *s) {
    if (!*s) {
        return 0;
    }
    int w = 0;
    for (; *s; s++) {
        char c = *s;
        if (c < 0x20 || c > 0x7e) {
            c=0x7f;
        }
        w += widths[(int)c - 0x20] + 1;
    }
    return w-1;
}
EOF

close OUT;
