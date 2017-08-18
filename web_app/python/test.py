#!/usr/bin/python
import sys, csv
import h5py
import json

# get the percentage of positive objects
fpath = sys.argv[1]

dict_clinicaldata = {}
slidename = []
pos = []

with open(fpath, 'rb') as csvfile:
     reader = csv.reader(csvfile, delimiter=',')
     idx = 0
     for row in reader:
         slidename.append(row[0])
         pos.append(row[1])


# Generate data to send to PHP
results = {'slidename': slidename, 'score':pos}
#results = {'score': dict_clinicaldata['test']}
jsonData = json.dumps(results, ensure_ascii = 'False')
print jsonData
