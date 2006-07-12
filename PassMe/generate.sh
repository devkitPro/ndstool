#!/usr/bin/sh
# Automatic generation script. Do not use.

DATZIP2FILE=romlist.zip
ROMLIST1=romlist_n.dat
ROMLIST2=romlist_u.dat
ROMLIST=romlist.dat
TMPFILE=info.tmp

NDS_MAIN_DIR=N:
#C:/public/nds
NDS_INCOMING_DIR=$NDS_MAIN_DIR/incoming
NDS_FOUND_DIR=$NDS_MAIN_DIR/found
NDS_NOTFOUND_DIR=$NDS_MAIN_DIR/notfound
PASSME_OUTPUT_DIR=$NDS_MAIN_DIR/passme

"C:/Windows/system32/subst.exe" N: \\\\akusho\\darkfader\\.mldonkey\\incoming\\nds

NDSTOOL=C:/work/DS/buildscripts/tools/nds/ndstool/ndstool

rm "$ROMLIST"

if [[ ! -f "$ROMLIST" ]]; then
	echo "downloading romlist..."

	# retrieve latest numbered DAT file
	DATZIPFILE=`curl -s --url "http://releases.pocketheaven.com/" | gawk '{ if (match($0, "PocketHeaven_NDS-Numbered_Release_List_Roms.*RC..zip")) print substr(\$0, RSTART, RLENGTH); }'`
	DATZIPURL="http://releases.pocketheaven.com/dats/$DATZIPFILE"
	wget -U "Mozilla" -O "$DATZIP2FILE" "$DATZIPURL"
	unzip -x "$DATZIP2FILE" "*.dat"
	rm "$DATZIP2FILE"
	mv PocketHeaven*.dat "$ROMLIST1"
	
	# retrieve latest unnumbered DAT file
	DATZIPFILE=`curl -s --url "http://releases.pocketheaven.com/" | gawk '{ if (match($0, "PocketHeaven_NDS-Other_Release_List_Roms.*RC..zip")) print substr(\$0, RSTART, RLENGTH); }'`
	DATZIPURL="http://releases.pocketheaven.com/dats/$DATZIPFILE"
	wget -U "Mozilla" -O "$DATZIP2FILE" "$DATZIPURL"
	unzip -x "$DATZIP2FILE" "*.dat"
	rm "$DATZIP2FILE"
	mv PocketHeaven*.dat "$ROMLIST2"

	# concatenate
	cat "$ROMLIST1" "$ROMLIST2" > "$ROMLIST"
	rm "$ROMLIST1" "$ROMLIST2"
else
	echo "Romlist already exist."
	#read -a yn -e -p "Delete ?" -n 1 && echo $REPLY
fi

# clean files
rm $NDS_INCOMING_DIR/*.nfo $NDS_INCOMING_DIR/*.jpg $NDS_INCOMING_DIR/*.png $NDS_INCOMING_DIR/*.diz

# 
(
	echo "--"
	echo "-- `date`"
	echo "--"
) >> generate.sql 

$NDSTOOL || exit
zip -L || exit

# rename and generate PassMe files
for NDS in $NDS_INCOMING_DIR/*.nds; do
	echo "$NDS"

	nice $NDSTOOL -v "$ROMLIST" -i "$NDS" > $TMPFILE
	TITLE=`gawk 'BEGIN { FS="\011"; } /Release title/ { print $2 }' $TMPFILE`
	INDEX=`gawk 'BEGIN { FS="\011"; } /Release index/ { gsub("^0*","",$2); print sprintf("%04u\n", $2); }' $TMPFILE`
	GAMECODE=`gawk 'BEGIN { FS="\011"; } /Game code/ { print substr($3,1,4) }' $TMPFILE`
	ROMVERSION=`gawk 'BEGIN { FS="\011"; } /ROM version/ { print substr($3,4,1) }' $TMPFILE`
	LONGFILENAME=`echo "$INDEX - $GAMECODE-$ROMVERSION - $TITLE" |
		gawk 'BEGIN { FS="/"; OFS="/"; } { gsub("[/\\\\?:<>|]","_",$(NF)); print $0 }' |
		gawk 'BEGIN { FS="\\\\"; OFS="\\\\"; } { gsub("[/\\\\?:<>|]","_",$(NF)); print $0 }'
	`
	SHORTFILENAME="$GAMECODE-$ROMVERSION"

	if [[ -n $TITLE ]]; then
		echo "$NDS is $LONGFILENAME" >&2
		(
			if [[ "$GAMECODE" != "NTRJ" ]]; then
				# Generate PassMe files and move
				# 
				nice $NDSTOOL -p "$NDS" passme.vhd passme.sav &&
				(
					rm passme.jed passme.xsvf passme.stapl passme.jam passme.jbc passme.sav passme.ngd passme.syr passme.ngc 2>&1 | echo -n
					echo Compiling VHDL...>&2
					nice xst -intstyle ise -ifn __projnav/passme.xst -ofn passme.syr &&
						nice ngdbuild -dd _ngo -uc passme.ucf -aul -p xc9500xl passme.ngc passme.ngd &&
						nice cpldfit -p xc9572xl-5-VQ44 -ofmt vhdl -optimize density -htmlrpt -loc on -slew fast -init low -inputs 54 -pterms 25 -unused float -power std -terminate keeper passme.ngd &&
						nice hprep6 -s IEEE1149 -n passme -i passme &&
						(
							echo loadcdf -file passme.ipf
							#echo setCable -port stapl -file passme.stapl
							echo setCable -port xsvf -file passme.xsvf
							echo Program -p 1 -e
							echo exit
						) | impact -batch &&
					(
						mv passme.stapl passme.jam
						#jbc passme.jam passme.jbc
						echo Zipping programming files...>&2
						zip -q -9 "$PASSME_OUTPUT_DIR/$SHORTFILENAME.zip" passme.vhd passme.jed passme.xsvf
						#passme.jam
						#passme.sav passme.gba
						#zip -q -9 "$PASSME_OUTPUT_DIR/$SHORTFILENAME.vhd.zip" passme.vhd
						#zip -q -9 "$PASSME_OUTPUT_DIR/$SHORTFILENAME.jed.zip" passme.jed
						#zip -q -9 "$PASSME_OUTPUT_DIR/$SHORTFILENAME.jam.zip" passme.jam
						#zip -q -9 "$PASSME_OUTPUT_DIR/$SHORTFILENAME.sav.zip" passme.sav
						#mv passme.jed "$PASSME_OUTPUT_DIR/$SHORTFILENAME.jed"
						#mv passme.jam "$PASSME_OUTPUT_DIR/$SHORTFILENAME.jam"
						##mv passme.sav "$PASSME_OUTPUT_DIR/$SHORTFILENAME.sav"
						mv "$NDS" "$NDS_FOUND_DIR/$LONGFILENAME.nds"
					)
				) || mv "$NDS" "$NDS_NOTFOUND_DIR/$LONGFILENAME.nds"
			fi

			cat $TMPFILE

		) | gawk '
			function strtonum2(a) { base=10; v=0; for (i=1; ; i++) { c=substr(a,i,1); if (c=="x") { base=16; continue; } if (c=="") break; if (hex[c]=="") break; v=v*base + hex[toupper(c)]; } return v; }
			BEGIN { FS="\011"; for (i=0; i<10; i++) hex[i]=i; hex["A"]=10; hex["B"]=11; hex["C"]=12; hex["D"]=13; hex["E"]=14; hex["F"]=15; }

			{ gsub("\015",""); _2=strtonum2($2); _3=strtonum2($3); }
	
			/Device capacity/ { DATA["romsize"]=lshift(1,17+_3) }
			/ROM version/ { DATA["romver"]=_3 }
			/Game code/ { DATA["gamecode"]=substr($3,1,4) }
			/Maker code/ { DATA["makercode"]=substr($3,1,2) }
			/Release index/ { DATA["releaseindex"]=_2 }
			/Release title/ { DATA["releasetitle"]=$2 }
			/Release group/ { DATA["releasegroup"]=$2 }
			/CRC32/ { DATA["crc32"]=substr($2,1,8) }
			/Secure area CRC/ { DATA["scrc"]=sprintf("%04X",_3) }
			/Header CRC/ { DATA["hcrc"]=sprintf("%04X",_3) }
	
			/Patch:/ { PATCH=PATCH $2 "=" $3 "," }
			/ARM9 patched entry address/ { DATA["passme_arm9"]=_2 }
	
			END {
				if (PATCH) DATA["passme_patches"] = PATCH;
				O = "REPLACE dsgame SET ";
				for (D in DATA)
				{
					if (C) O=O ","; C=1;
					O=O D "=\"" DATA[D] "\"";
				}
				if (DATA["releasetitle"]) print O ";"
			}
		' >> generate.sql

	fi

	rm $TMPFILE
done


# find highest rom number
HIGHEST=`gawk '
	function strtonum2(a) { base=10; v=0; for (i=1; ; i++) { c=substr(a,i,1); if (c=="x") { base=16; continue; } if (c=="") break; if (hex[c]=="") break; v=v*base + hex[toupper(c)]; } return v; }
	BEGIN { FS="\xAC"; for (i=0; i<10; i++) hex[i]=i; hex["A"]=10; hex["B"]=11; hex["C"]=12; hex["D"]=13; hex["E"]=14; hex["F"]=15; }
	$2 ~ /Demo/ { $2="" }
	$2 ~ /Nuke/ { $2="" }
	{ N=strtonum2(substr($2,1,4)); if (N > HIGHEST) HIGHEST = N; }
	END { print HIGHEST }
' "$ROMLIST"`

# list my NDS files and find out the missing ones
ls $NDS_FOUND_DIR $NDS_NOTFOUND_DIR | sort | gawk -v HIGHEST=$HIGHEST '
	function strtonum2(a) { base=10; v=0; for (i=1; ; i++) { c=substr(a,i,1); if (c=="x") { base=16; continue; } if (c=="") break; if (hex[c]=="") break; v=v*base + hex[toupper(c)]; } return v; }
	BEGIN { for (i=0; i<10; i++) hex[i]=i; hex["A"]=10; hex["B"]=11; hex["C"]=12; hex["D"]=13; hex["E"]=14; hex["F"]=15; }

	{ gsub("^0*","",$1); N=strtonum2($1); roms[N]=$0; }
	END { for (I=265; I<=HIGHEST; I++) if (!roms[I]) print "Missing: " I roms[I]; }
'

#du found/*.nds notfound/*.nds
