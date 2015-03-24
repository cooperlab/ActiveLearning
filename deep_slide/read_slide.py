"""
Uses OpenSlide to extract training data from .svs whole-slide-images. Extracts small sub-images
of cells at specified {x_i, y_i}.
"""

from __future__ import print_function
import json
from multiprocessing import Process, JoinableQueue
from openslide import open_slide, ImageSlide
from openslide.deepzoom import DeepZoomGenerator
from optparse import OptionParser
import os
import re
import shutil
import sys
import csv
from unicodedata import normalize


Slides=[]
Classes=[]
Xs=[]
Ys=[]
fname_map = {}

def print_pixel(x, y, img):    
	pixel = img.getpixel((x, y))
	print("pixel at [{0}, {1}] = ({2})\n".format(x, y, pixel))      
def getInfo(i ):
	print("filename: {0}, path: {1}\n".format(Slides[i], fname_map[Slides[i]]))
	slidepath = fname_map[Slides[i]]
	_slide = open_slide(slidepath)
	#print slide information
	'''
	print("Information about the image: \n"
			"level_count= {0}\n"  
			"dimensions= {1}\n" 
			"level_dimensions= {2}\n" 
			"level_downsamples= {3}\n" 
			"properties= {4}\n"
			"associated_images= {5}\n".format(_slide.level_count, _slide.dimensions, 
										_slide.level_dimensions, _slide.level_downsamples, 
										_slide.properties, _slide.associated_images))   

	'''    										 
	#read region
	x = int(float(Xs[i]))
	y = int(float(Ys[i]))
	print("x={0},y={1}\n".format(x,y))
	size = 50;
	#print("half={0}".format(size/2))
	img_region = _slide.read_region([x-size/2,y-size/2],0,[size,size])
	filename = Slides[i]
	directory = '{1}/{0}'.format(Classes[i], size)
	if not os.path.exists(directory):
		os.makedirs(directory)
		
	img_region.save("{3}/{0}_{1}_{2}.jpg".format(filename[:-4], x, y, directory), "JPEG")

	_slide.close()										

										
if __name__ == '__main__':

    parser = OptionParser(usage='Usage: %prog <slide>')
    (opts, args) = parser.parse_args()
  
    #read classes
    with open('NuclearClasses.txt', 'r') as f:
    	next(f)
    	reader=csv.reader(f, delimiter='\t')
    	for Slide,Class,X,Y in reader:
    		Slides.append(Slide)
    		Classes.append(Class)
    		Xs.append(X)
    		Ys.append(Y)
    #print(Slides)
    #print(Classes)
    #print(Xs)
    #print(Ys)    
    print("{0}\t{1}\t{2}\t{3}".format(Slides[0], Classes[0], Xs[0], Ys[0]))
    print("{0}\t{1}\t{2}\t{3}".format(Slides[1], Classes[1], Xs[1], Ys[1]))
    

    fname_map['TCGA-02-0006-01Z-00-DX2.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.7.0.0/TCGA-02-0006-01Z-00-DX2.svs'
    fname_map['TCGA-02-0010-01Z-00-DX1.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.7.0.0/TCGA-02-0010-01Z-00-DX1.svs'
    fname_map['TCGA-02-0010-01Z-00-DX3.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.7.0.0/TCGA-02-0010-01Z-00-DX3.svs'
    fname_map['TCGA-02-0015-01Z-00-DX1.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.7.0.0/TCGA-02-0015-01Z-00-DX1.svs'
    fname_map['TCGA-06-0154-01Z-00-DX1.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.8.0.0/TCGA-06-0154-01Z-00-DX1.svs'
    fname_map['TCGA-06-0154-01Z-00-DX3.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.8.0.0/TCGA-06-0154-01Z-00-DX3.svs'
    fname_map['TCGA-06-0171-01Z-00-DX3.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.8.0.0/TCGA-06-0171-01Z-00-DX3.svs'
    fname_map['TCGA-06-0195-01Z-00-DX4.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.9.0.0/TCGA-06-0195-01Z-00-DX4.svs'
    fname_map['TCGA-06-0648-01Z-00-DX1.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.6.0.0/TCGA-06-0648-01Z-00-DX1.svs'
    fname_map['TCGA-12-0618-01Z-00-DX1.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.6.0.0/TCGA-12-0618-01Z-00-DX1.svs'
    fname_map['TCGA-12-0618-01Z-00-DX4.xml'] = '/data2/Images/bcrTCGA/diagnostic_block_HE_section_image/intgen.org_GBM.tissue_images.6.0.0/TCGA-12-0618-01Z-00-DX4.svs'
    
    fname_map['astroII.2.xml'] = '/home/wsi5/Documents/astroII.2.ndpi'
    fname_map['normal.2.xml'] = '/data2/Images/NuclearClassification/normal.2.ndpi'
    #fname_map['normal.3.xml'] = '/data2/Images/NuclearClassification/normal.3.ndpi' 
    length = int(len(Slides))
    print(length) 
    for i in range(0,length):    
		if fname_map.get(Slides[i]) != None:
			getInfo(i)
    

