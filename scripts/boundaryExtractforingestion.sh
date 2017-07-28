#!/bin/bash

export PATH=./:$PATH

if [ -z "$1" ]; then
	echo usage: $0 '<source dir> <dest file>'
	exit
fi


if [ -z "$2" ]; then
	echo useage $0 '<source dir> <dest file>'
	exit
fi


for slide in $( ls $1/*.seg*.txt ); do
	echo $slide;
	extract-GBM-nuclei.py $slide >> $2
done
