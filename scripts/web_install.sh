#!/bin/bash

if [ -z "$1" ]; then
	echo usage $0 '<web root dir>'
	exit
fi

VALS_DIR=$1'/VALS'
mkdir $VALS_DIR
mkdir $VALS_DIR'/datasets'
mkdir $VALS_DIR'/trainingsets'

mkdir $VALS_DIR'/heatmaps'

# For reports
mkdir $VALS_DIR'/trainingsets/tmp'
chown www-data:www-data $VALS_DIR'/trainingsets/tmp'
chmod 777 $VALS_DIR'/trainingsets/tmp'

cp -r ../web_app/* $VALS_DIR

