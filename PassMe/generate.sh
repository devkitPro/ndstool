#!/usr/bin/sh
# Automatic generation script. Do not use.

PATH=..:$PATH
DATZIP2FILE=romlist.zip
NUMBERED=romlist_n.dat
UNNUMBERED=romlist_u.dat
TMPFILE=info.tmp
NDSDIR=C:/public/nds
NDSFOUNDDIR=$NDSDIR/found
NDSNOTFOUNDDIR=$NDSDIR/notfound
PASSMEDIR=C:/work/DS/buildscripts/tools/nds/ndstool/PassMe
PASSMEDIR2=C:/public/nds/passme

# retrieve latest numbered DAT file
DATZIPFILE=`curl -s --url "http://releases.pocketheaven.com/" | gawk '{ if (match($0, "PocketHeaven_NDS-Numbered_Release_List_Roms.*RC..zip")) print substr(\$0, RSTART, RLENGTH); }'`
DATZIPURL="http://releases.pocketheaven.com/dats/$DATZIPFILE"
wget -U "Mozilla" -O "$DATZIP2FILE" "$DATZIPURL"
unzip -x "$DATZIP2FILE" "*.dat"
rm "$DATZIP2FILE"
mv PocketHeaven*.dat "$NUMBERED"

# retrieve latest unnumbered DAT file
DATZIPFILE=`curl -s --url "http://releases.pocketheaven.com/" | gawk '{ if (match($0, "PocketHeaven_NDS-Other_Release_List_Roms.*RC..zip")) print substr(\$0, RSTART, RLENGTH); }'`
DATZIPURL="http://releases.pocketheaven.com/dats/$DATZIPFILE"
wget -U "Mozilla" -O "$DATZIP2FILE" "$DATZIPURL"
unzip -x "$DATZIP2FILE" "*.dat"
rm "$DATZIP2FILE"
mv PocketHeaven*.dat "$UNNUMBERED"

# clean files
rm $NDSDIR/*.nfo $NDSDIR/*.jpg $NDSDIR/*.png $NDSDIR/*.diz

# rename and generate PassMe files
for NDS in $NDSDIR/*.nds; do
	echo "$NDS"

	# numbered?
	nice ndstool -v -i "$NDS" "$NUMBERED" > $TMPFILE
	TITLE=`gawk 'BEGIN { FS="\011"; } /Release title/ { print $2 }' $TMPFILE`
	INDEX=`gawk 'BEGIN { FS="\011"; } /Release index/ { print $2 }' $TMPFILE`
	GAMECODE=`gawk 'BEGIN { FS="\011"; } /Game code/ { print substr($3,1,4) }' $TMPFILE`
	ROMVERSION=`gawk 'BEGIN { FS="\011"; } /ROM version/ { print substr($3,4,1) }' $TMPFILE`
	FILENAME="$INDEX - $GAMECODE-$ROMVERSION - $TITLE"
	if [[ -n $TITLE ]]; then
		echo $NDS is $TITLE
		nice ndstool -p "$NDS" "$PASSMEDIR/passme.vhd" "$PASSMEDIR2/$FILENAME.sav" &&
		(
			CWD=`pwd`
			cd "$PASSMEDIR"
			rm passme.jed
			nice xst -intstyle ise -ifn __projnav/passme.xst -ofn passme.syr &&
				nice ngdbuild -dd _ngo -uc passme.ucf -aul -p xc9500xl passme.ngc passme.ngd &&
				nice cpldfit -p xc9572xl-5-VQ44 -ofmt vhdl -optimize speed -htmlrpt -loc on -slew fast -init low -inputs 54 -pterms 25 -unused float -power std -terminate keeper passme.ngd &&
				nice hprep6 -s IEEE1149 -n passme -i passme &&
				mv passme.jed "$PASSMEDIR2/$FILENAME.jed"
			cd "$CWD"
			mv "$NDS" "$NDSFOUNDDIR/$FILENAME.nds"
		) || mv "$NDS" "$NDSNOTFOUNDDIR/$FILENAME.nds"
	else

		# unnumbered?
		nice ndstool -v -i "$NDS" "$UNNUMBERED" > $TMPFILE
		TITLE=`gawk 'BEGIN { FS="\011"; } /Release title/ { print $2 }' $TMPFILE`
		GAMECODE=`gawk 'BEGIN { FS="\011"; } /Game code/ { print substr($3,1,4) }' $TMPFILE`
		ROMVERSION=`gawk 'BEGIN { FS="\011"; } /ROM version/ { print substr($3,4,1) }' $TMPFILE`
		FILENAME="$GAMECODE-$ROMVERSION - $TITLE"
		if [[ -n $TITLE ]]; then
			echo $NDS is $TITLE
			mv "$NDS" "$NDSDIR/$FILENAME.nds"
		fi

	fi
	rm $TMPFILE
done
