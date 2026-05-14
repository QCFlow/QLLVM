# This code is part of QLLVM.
#
# (C) Copyright QCFlow 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE file in the root directory
# of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os


def show(path):
    import pandas as pd
    import matplotlib.pyplot as plt

    df = pd.read_csv(path)
    plt.rc("font",family="AR PL UKai CN")

    colors = ['#2b7bba', '#ff7f0e']
    titles = ['Gate count reduction(%)', 'Depth reduction(%)']
    y_labels = ['reduction(%)', 'reduction(%)']
    
    df['file name'] = df['file name'].apply(lambda x: x.split('_')[0])
    
    avg_data = df.groupby('file name').agg({
        'gate count ratio': 'mean',
        'depth ratio': 'mean'
    }).reset_index()
    
    avg_values = {
        'gate count ratio': df['gate count ratio'].mean(),
        'depth ratio': df['depth ratio'].mean()
    }
    
    fig, axes = plt.subplots(1, 2, figsize=(12, 6))
    axes = axes.flatten()

    for i, (col, title, ylabel, color) in enumerate(zip(
        ['gate count ratio', 'depth ratio'],
        titles, y_labels, colors
    )):
        ax = axes[i]
        
        bars = ax.bar(
            x=avg_data['file name'],
            height=avg_data[col],
            color=color,
            alpha=0.7,
            width=0.6
        )
        
        avg_line = ax.axhline(
            y=avg_values[col], 
            color="#000000", 
            linestyle='--', 
            linewidth=2,
            alpha=0.7
        )
        
        ax.text(
            len(avg_data)-0.5, avg_values[col], 
            f'average: {avg_values[col]:.2f}%',
            color="#000000",
            ha='right', 
            va='center',
            fontsize=12,
            bbox=dict(facecolor='white', alpha=0.7, edgecolor='none')
        )
        
        ax.set_title(title, pad=12, fontsize=14, fontweight='bold')
        ax.set_ylabel(ylabel, fontsize=11, fontweight='bold')
        ax.set_xticks(range(len(avg_data)))
        ax.set_xticklabels(avg_data['file name'], rotation=45, ha='right')
        
        ax.axhline(y=0, color='black', linestyle='--', linewidth=1.2)
        
    
    plt.tight_layout(pad=3.0)
    plt.show()

def show_CN(path):
    import pandas as pd
    import matplotlib.pyplot as plt

    df = pd.read_csv(path)
    plt.rc("font",family="AR PL UKai CN")

    colors = ['#2b7bba', '#ff7f0e']
    titles = ['门个数降低百分比(%)', '线路深度降低百分比(%)']
    y_labels = ['百分比(%)', '百分比(%)']
    
    df['file name'] = df['file name'].apply(lambda x: x.split('_')[0])
    
    avg_data = df.groupby('file name').agg({
        'gate count ratio': 'mean',
        'depth ratio': 'mean'
    }).reset_index()
    
    avg_values = {
        'gate count ratio': df['gate count ratio'].mean(),
        'depth ratio': df['depth ratio'].mean()
    }
    
    fig, axes = plt.subplots(1, 2, figsize=(12, 6))
    axes = axes.flatten()

    for i, (col, title, ylabel, color) in enumerate(zip(
        ['gate count ratio', 'depth ratio'],
        titles, y_labels, colors
    )):
        ax = axes[i]
        
        bars = ax.bar(
            x=avg_data['file name'],
            height=avg_data[col],
            color=color,
            alpha=0.7,
            width=0.6
        )
        
        avg_line = ax.axhline(
            y=avg_values[col], 
            color="#000000", 
            linestyle='--', 
            linewidth=2,
            alpha=0.7
        )
        
        ax.text(
            len(avg_data)-0.5, avg_values[col], 
            f'平均值: {avg_values[col]:.2f}%',
            color="#000000",
            ha='right', 
            va='center',
            fontsize=12,
            bbox=dict(facecolor='white', alpha=0.7, edgecolor='none')
        )
        
        ax.set_title(title, pad=12, fontsize=14, fontweight='bold')
        ax.set_ylabel(ylabel, fontsize=11, fontweight='bold')
        ax.set_xticks(range(len(avg_data)))
        ax.set_xticklabels(avg_data['file name'], rotation=45, ha='right')
        
        ax.axhline(y=0, color='black', linestyle='--', linewidth=1.2)
        
    
    plt.tight_layout(pad=3.0)
    plt.show()
