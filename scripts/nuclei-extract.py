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
	boundary = items[len(items) - 1]
	boundary = boundary.rstrip()

	print items[0], "\t", items[1], "\t", items[2], "\t",
	
	#
	#	Strip semicolons
	#
	boundary = string.split(boundary, ';')
	for vertex in boundary:
		if len(vertex):
			print vertex[0:vertex.index('.')] + vertex[vertex.index('.')+2:-2],

	print
	
	
