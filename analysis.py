import pandas as pd
import matplotlib.pyplot as plt
import sys

# Define the initial heap size from the C++ program in bytes.
# This makes the script easier to maintain if you change the C++ code.
INITIAL_HEAP_SIZE_BYTES = 20 * 1024 * 1024

def plot_fragmentation_data(csv_filepath="heap_fragmentation_stats.csv"):
    """
    Reads heap fragmentation data from a CSV file and generates plots
    based on user-defined metrics.

    Args:
        csv_filepath (str): The path to the CSV file generated by the C++ program.
    """
    try:
        # Read the data from the CSV file into a pandas DataFrame.
        df = pd.read_csv(csv_filepath)
        print(f"Successfully loaded data from '{csv_filepath}'.")
    except FileNotFoundError:
        print(f"Error: The file '{csv_filepath}' was not found.")
        print("Please run the C++ program first to generate the data file.")
        sys.exit(1)
    except pd.errors.EmptyDataError:
        print(f"Error: The file '{csv_filepath}' is empty.")
        sys.exit(1)
    except KeyError as e:
        print(f"Error: Missing expected column in CSV: {e}")
        print("Please ensure you are using the updated C++ program that outputs the 'TotalUserRequested' column.")
        sys.exit(1)

    # --- Calculate the requested metrics ---
    
    # Metric 1: Total Occupied Space = Initial Heap Size - Total Free Space
    # This shows how "full" the heap is with both useful data and all forms of waste.
    df['Occupied_Memory'] = INITIAL_HEAP_SIZE_BYTES - df['TotalFree_Bytes']

    # Metric 2: Total Wasted Space = Initial Size - Free Space - User Requested Space
    # This represents all memory that is not free and not holding useful data.
    # It's the sum of internal fragmentation and heap management overhead.
    df['Total_Waste'] = INITIAL_HEAP_SIZE_BYTES - df['TotalFree_Bytes'] - df['TotalUserRequested']


    # --- Create the Plots ---

    # Create a figure and a set of 2 subplots arranged vertically.
    fig, axes = plt.subplots(2, 1, figsize=(12, 12), sharex=True)
    fig.suptitle('Custom Heap Fragmentation Analysis', fontsize=18, fontweight='bold')

    # Plot 1: Occupied Heap Space
    axes[0].plot(df['Time'], df['Occupied_Memory'], color='darkcyan', marker='.', linestyle='-')
    axes[0].set_title('Total Occupied Heap Space Over Time', fontsize=14)
    axes[0].set_ylabel('Occupied Memory (Bytes)')
    axes[0].grid(True, which='both', linestyle='--', linewidth=0.5)
    axes[0].ticklabel_format(style='sci', axis='y', scilimits=(0,0)) # Use scientific notation
    axes[0].axhline(y=INITIAL_HEAP_SIZE_BYTES, color='r', linestyle='--', label=f'Max Heap Size ({INITIAL_HEAP_SIZE_BYTES/1024/1024:.0f}MB)')
    axes[0].legend()


    # Plot 2: Total Wasted Space (Internal Fragmentation + Overhead)
    axes[1].plot(df['Time'], df['Total_Waste'], color='orangered', marker='.', linestyle='-')
    axes[1].set_title('Total Wasted Space (Fragmentation + Overhead) Over Time', fontsize=14)
    axes[1].set_xlabel('Time (Simulation Steps)')
    axes[1].set_ylabel('Wasted Memory (Bytes)')
    axes[1].grid(True, which='both', linestyle='--', linewidth=0.5)
    axes[1].ticklabel_format(style='sci', axis='y', scilimits=(0,0))


    # Adjust layout to prevent titles and labels from overlapping.
    plt.tight_layout(rect=[0, 0, 1, 0.96])

    # Save the entire figure to a single PNG file.
    output_filename = 'custom_fragmentation_analysis.png'
    plt.savefig(output_filename)
    print(f"\nPlot saved successfully as '{output_filename}'")

if __name__ == '__main__':
    # To run this script, you need pandas and matplotlib:
    # pip install pandas matplotlib
    plot_fragmentation_data()
