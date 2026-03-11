import os
import sys
import argparse
import numpy as np

from pathlib import Path

parser = argparse.ArgumentParser(description='Verify ExportAnimatedSpreadSheetView output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

expected_frames = [ 5, 7, 9, 11, 13, 15 ]
# this is not exactly the header from the file, but how numpy sanitize it
expected_columns = [
    'Structured_Coordinates0',
    'Structured_Coordinates1',
    'Structured_Coordinates2',
    'SpatioTemporalHarmonics'
    ]

tested_column = 'SpatioTemporalHarmonics'
expected_values = { 5: 0.999615, 7: -0.00387539, 9: -0.781564, 11: -3.75107, 13: -5.42529, 15: -0.998741 }

# Open CSV file written by the XML test and verify the column names.
test_output_dir = args.T
test_name = args.N
csv_root_file_name = test_output_dir + os.sep + test_name

# first loop to check that all files were written
for timestep in expected_frames:
    csv_file_name = f"{csv_root_file_name}.{str(timestep)}.csv"
    file_path = Path(csv_file_name)
    if not file_path.is_file():
        print(f"Error: missing expected output file: {str(file_path)}")
        sys.exit(1)

# check file content
for timestep in expected_frames:
    csv_file_name = f"{csv_root_file_name}.{str(timestep)}.csv"
    file_path = Path(csv_file_name)
    # assuming datafile.txt is a CSV file with the 1st row being the names for the columns
    data = np.genfromtxt(file_path, dtype=None, names=True, delimiter=',', autostrip=False)

    header_columns = data.dtype.names
    if len(expected_columns) != len(header_columns):
        print(f"Error: Number of columns in {file_path} does not match expected number of columns ({len(header_columns)} != {len(expected_columns)})")
        sys.exit(1)

    for (expected, read) in zip(expected_columns, header_columns):
        if expected != read:
            print(f"Error: Expected column {expected}, read column {read}")
            sys.exit(1)

    if data[tested_column][0] != expected_values[timestep]:
        print(f"Error: Expected {expected_values[timestep]} as harmonic value, but read {data[tested_column][0]}")
        sys.exit(1)


print(f"{test_name} script passed")
