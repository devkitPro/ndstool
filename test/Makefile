PATH := ./:../:$(PATH)


repack_test: files repacked.nds files2

files: original.nds Makefile ../ndstool
	-@rm -r files
	ndstool -x $< -7 arm7.bin -9 arm9.bin -h header.bin -t banner.bin -y overlays -d files

repacked.nds: arm7.bin arm9.bin header.bin files Makefile ../ndstool
	ndstool -c $@ -7 arm7.bin -9 arm9.bin -h header.bin -t banner.bin -y overlays -d files

files2: repacked.nds Makefile ../ndstool
	-@rm -r files2
	ndstool -x $< -d files2



#######################
# old stuff below
#######################

test:
	$(MAKE) -C ..
	ndstool -c test.nds -7 arm7.bin -9 arm9.bin -b icon.bmp "Hello;World;:)" -d files -r9 0x02000000 -e9 0x02000800 -g AMFE -s -l -i
	# -m DF
	#ndstool -l test.nds
	#-s -i -l
	#ndstool -s test.nds
	#ndstool -i test.nds

#-r9 0x02000000 -r7 0x02380000

# ndstool
ndstool_create:
	#ndstool -x nibbles.nds -7 arm7.bin -9 arm9.bin
	#-h header.bin
	ndstool -v -c nibbles2.nds -7 arm7.bin -9 arm9.bin -r9 0x02004000 -r7 0x02380000 -b icon.bmp "Hello;World;:)"
	 #-d files -y overlays -y9 y9.bin -y7 y7.bin
	#ndstool -v -x nibbles2.nds -d files_extract -y overlays_extract -y9 y9_extract.bin -y7 y7_extract.bin
# -o blaat.bmp
	
	# -d nibbles
	ndstool -i nibbles2.nds
	ndstool -l nibbles2.nds
	#ndstool -x nibbles2.nds -d extract

	#-rm -rf files2
	#ndstool -x nibbles2.nds -d files2

	#-rm metroid.nds
	#-h metroid_header.bin -t metroid_icontitle.bin -7 metroid_ARM7.bin -9 metroid_ARM9.bin
	#ndstool -c metroid.nds -d metroid_files

	#ndstool -l metroid.nds

	#-rm -r extract
	#ndstool -x metroid.nds -d extract

#0x2D2A8


ndstool_release: ndstool
	ndstool -c nibbles2.nds -7 arm7.bin -9 arm9.bin -d nibbles
	ndstool -i nibbles2.nds

ndstool_debug: OPTIONS := -D_NDSTOOL_P_H
ndstool_debug: ndstool
	gdb --args ndstool -c nibbles2.nds -7 arm7.bin -9 arm9.bin -d nibbles

ndstool: ndstool.cpp ndstool.p.h
	g++ -mno-cygwin -O2 -s -o ndstool ndstool.cpp -lelf -lDevIL -lILU -L /usr/local/lib -I /usr/local/include $(OPTIONS) -I C:\public\Devil-SDK-1.6.7\include -L C:\public\Devil-SDK-1.6.7\lib

ndstool_info: ndstool
	ndstool -i "0001.Metroid.Prime.Hunters.First.Hunt.(DF-v2).nds"
