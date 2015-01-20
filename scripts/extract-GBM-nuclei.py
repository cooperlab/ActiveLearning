#!/usr/bin/env python


import sys
import string


if len(sys.argv) != 2:
	print "Usage:", sys.argv[0], " <segmentation file> "
	exit(1)


data = open(sys.argv[1], 'r').readlines()
data = data[1:]   		# Remove header

for nuclei in data:

	items = string.split(nuclei, '\t')
	#
	# Remove extra from slide name
	nameSplit = string.split(items[0], '.')
	
	if float(items[3]) == 40.0:
		#
		#	Need to adjust for 40x slides by multiplying by 2
		#
		x = float(items[1]) * 2.0
		y = float(items[2]) * 2.0

		print nameSplit[0], "\t", x, "\t", y, "\t", 
		vertices = string.split(items[len(items) - 1], ';')

		#
		#	Strip semicolons
		#
		for point in vertices:

			coords = string.split(point, ',')
			if len(coords) == 2:
				x = float(coords[0]) * 2.0
				y = float(coords[1]) * 2.0
				print  str(x) + "," + str(y),
		print

	else:

		print nameSplit[0], "\t", items[1], "\t", items[2], "\t",
		vertices = string.split(items[len(items) - 1], ';')

		#
		#	Strip semicolons
		#
		for point in vertices:
			coords = string.split(point, ',')
			if len(coords) == 2:
				print point[0:point.index('.')] + point[point.index('.')+2:-2],
	
		print



