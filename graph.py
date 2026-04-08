import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def plot_benchmark(csv_file, output_file=None):
    print(f"Чтение файла: {csv_file}")
    df = pd.read_csv(csv_file)
    
    print(f"Колонки: {list(df.columns)}")
    print(f"Всего записей: {len(df)}")
    
    print("\nСтатистика по времени (мс):")
    print(df['time_ms'].describe())
    
    graphs = df['graph_name'].unique()
    print(f"\nНайдено графов: {len(graphs)}")
    print(f"Графы: {graphs}")

    fig, ax = plt.subplots(figsize=(8, 9))
    
    graph_to_x = {graph: i for i, graph in enumerate(graphs)}
    x_positions = [graph_to_x[g] for g in df['graph_name']]
    
    colors = plt.cm.tab10(np.linspace(0, 1, len(graphs)))
    graph_to_color = {graph: colors[i] for i, graph in enumerate(graphs)}
    
    np.random.seed(10)
    jitter_strength = 0.2
    x_jitter = np.random.uniform(-jitter_strength, jitter_strength, size=len(df))
    x_plot = np.array(x_positions) + x_jitter
    
    for graph in graphs:
        mask = df['graph_name'] == graph
        x_pos = [graph_to_x[graph]] * mask.sum()
        x_plot = np.array(x_pos) + x_jitter[mask]
        
        plt.scatter(x_plot, df.loc[mask, 'time_ms'],
                   alpha=0.09,
                   s=18,
                   c=[graph_to_color[graph]])
        
    for i, graph in enumerate(graphs):
        graph_data = df[df['graph_name'] == graph]
        if len(graph_data) > 0:
            median_val = graph_data['time_ms'].median()
            mean_val = graph_data['time_ms'].mean()
            
            ax.hlines(median_val, i - 0.3, i + 0.3, 
                     colors='black', linestyles='-', linewidth=2.5,
                     label='Median' if i == 0 else "")
            
            ax.hlines(mean_val, i - 0.3, i + 0.3, 
                     colors='black', linestyles='--', linewidth=2.5,
                     label='Mean' if i == 0 else "")
    
    ax.set_xticks(range(len(graphs)))
    ax.set_xticklabels(graphs, rotation=45, ha='right')
    ax.set_ylabel('Execution time (ms)', fontsize=12)  # ← подпись в мс
    
    ax.grid(True, alpha=0.4, linestyle='--', linewidth=0.8)
    
    ax.set_yscale('log')
    
    handles, labels = ax.get_legend_handles_labels()
    by_label = dict(zip(labels, handles))  # убираем дубликаты
    ax.legend(by_label.values(), by_label.keys(), loc='upper right')
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"График сохранен в {output_file}")
    else:
        plt.show()

def main():
    csv_file = "c_alias_bench_result.csv"
    output_file = "c_alias_bench_result.png"
    
    if not os.path.exists(csv_file):
        print(f"Ошибка: файл {csv_file} не найден")
        sys.exit(1)
    
    plot_benchmark(csv_file, output_file)

if __name__ == "__main__":
    main()