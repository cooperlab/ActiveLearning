import pandas as pd
import sys,os
import json
import numpy as np
from lifelines.statistics import logrank_test
data = sys.argv[1]
df = pd.read_json(data)
df.columns = df.columns.str.replace('\r','')
values = df['value']
gp1_tm = np.array(values[0]).astype(float)
gp1_evt = np.array(values[1]).astype(float)
gp2_tm = np.array(values[2]).astype(float)
gp2_evt = np.array(values[3]).astype(float)
results = logrank_test(gp1_tm, gp2_tm, gp1_evt, gp2_evt)
pvalue = round(results.p_value, 6)
# pvalue = results.p_value
lst = {"logrank_p" : pvalue}
json_last = json.dumps(lst, ensure_ascii = 'false')
print json_last
