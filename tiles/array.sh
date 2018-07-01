#!/bin/bash
rm image_headers.h
ls *_rotated.bmp | sed 's/.bmp//g' > list

while read filename;do
	width=`identify ${filename}.bmp | awk '{print $3}' | awk -F"x" '{print $1}'`
	height=`identify ${filename}.bmp | awk '{print $3}' | awk -F"x" '{print $2}'`
	size=`echo $width $height | awk '{print $1*$2*3}'`  
	echo "const unsigned char ${filename}[${size}] = {" >> image_headers.h
	./bmp.o ${filename}.bmp >> image_headers.h
	echo "};" >> image_headers.h
done < list
