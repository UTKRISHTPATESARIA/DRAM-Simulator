#!/usr/bin/env python
# coding: utf-8

# In[25]:


import os
import sys
import json
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


# In[26]:


plt.rcParams.update({'font.weight': 'bold'})
os.makedirs('Results', exist_ok=True)


# In[27]:


valid_columns = ['total_energy',             
                 
                 'average_power',
                 'average_bandwidth',
                 'average_read_latency',
                 'average_write_latency', # f

                 'num_act_cmds',
                 'num_pre_cmds',

                 'num_read_row_hits',
                 'num_write_row_hits', 
    
                 'rank0_bank_parallelism',
                 'rank1_bank_parallelism',
                 'avg_bank_parallelism',
]


# In[28]:


def get_avg_cycle(data, key):
    c_sum = 0
    count = 0
    temp_data = data[key]
    for key, value in temp_data.items():
        c_sum += int(key)*value
        count += value
    return c_sum/count


# In[29]:


def get_mmry_time(data):
    read_count = data['num_reads_done']
    write_count = data['num_writes_done']
    read_lat = data['average_read_latency']
    write_lat = data['average_write_latency']
    total_count = read_count + write_count
    avg_mmr_time = read_count/total_count*read_lat + write_count/total_count*write_lat 
    return avg_mmr_time


# In[30]:


def get_bank_parallelism(data):
    b1_idle_cycles = data['all_bank_idle_cycles']['0']
    b2_idle_cycles = data['all_bank_idle_cycles']['1']
    r0_bank_parallel = 1-(b1_idle_cycles/data['num_cycles'])
    r1_bank_parallel = 1-(b2_idle_cycles/data['num_cycles'])
    avg_bank_parallel = 1-((b1_idle_cycles + b2_idle_cycles)/(2*data['num_cycles']))
    return r0_bank_parallel, r1_bank_parallel, avg_bank_parallel


# In[31]:


path = './bench3-results'
current_bench = 'Benchmark-3'
final_data = pd.DataFrame()
for file in os.listdir(path):
    if file.endswith('.json'):
        _, row_type, mem_type, addr_type = file.split('.')[0].split('-')
        data = json.load(open(os.path.join(path, file)))['0']
        data['average_write_latency'] = get_avg_cycle(data, 'write_latency')
        r0_b_p, r1_b_p, a_b_p = get_bank_parallelism(data)
        data['avg_bank_parallelism'] = a_b_p
        data['avg_memory_access_time'] = get_mmry_time(data)
        valid_data = {key:value for key, value in data.items() if key in valid_columns}
        temp_df = pd.DataFrame(valid_data, index=[0])
        temp_df['ROW_POLICY'] = row_type
        temp_df['SCH_POLICY'] = mem_type
        temp_df['ADR_POLICY'] = addr_type
        temp_df['Benchmark'] = current_bench 
        temp_df['Group_Column'] = temp_df['SCH_POLICY'] + '-' + temp_df['ROW_POLICY'] + '-' + temp_df['ADR_POLICY']
        final_data = final_data.append(temp_df, ignore_index=True)


# In[33]:


def generate_plots(data, column_values, group_by_columns, file_name):
    title = f'Plot for {file_name} file for different policies'
    for label_name in column_values:
        sns.set(rc={'figure.figsize': (10, 10)})
        b = sns.barplot(data = data, y = group_by_columns, x = label_name, log=True)
        file_name_to_save = f"{file_name}-{label_name}.png"
        path = os.path.join('./Results/', file_name_to_save)
        for container in b.containers:
            b.bar_label(container)
        b.set_xlabel('')
        b.set_ylabel('')
        plt.xlabel(label_name, fontweight='bold')
        plt.title(title, fontweight='bold')
        plt.setp(b.get_xticklabels(), rotation=45)
        plt.tight_layout()
        b.figure.savefig(path)
        plt.close()


# In[34]:


generate_plots(final_data, list(final_data.columns[:-5]), 'Group_Column', current_bench)


# In[36]:


final_data.to_csv(f'{current_bench}_final_data.csv', sep=',', index=False)

