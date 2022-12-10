import modin.pandas as pd
import random 
import sys
import os

def scale_column(data, old_column_name, new_column_name, scaling_factor):
    last_value = data.iloc[-1][old_column_name]
    first_value = data.iloc[0][old_column_name]
    mean_diff = (last_value - first_value)/data.shape[0]
    data[new_column_name] = (scaling_factor * (data[old_column_name] - first_value)/(mean_diff) - (random.randrange(0, 2)))
    data.iloc[0][new_column_name] = 0
    data[new_column_name] = data[new_column_name].astype('int')
    return data

traces_file_path = '.'
traces_file_name = sys.argv[1]
new_traces_file_name = f'new_{traces_file_name}'
old_traces_file = os.path.join(traces_file_path, traces_file_name)
new_traces_file = os.path.join(traces_file_path, new_traces_file_name)
df = pd.read_csv(old_traces_file, delimiter=' ', header=None)
df_n = scale_column(df, 2, 3, float(sys.argv[2]))
df_n.drop(columns=[2], inplace=True)
df_n.to_csv(new_traces_file, sep=' ', index=False, header=False)
