import os
import sys
import argparse

parser = argparse.ArgumentParser(description='Verify ExportMultiblockFieldDataSpreadsheet output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

expected_columns = [
    "Block Name",
    "Point ID",
    "Points_0",
    "Points_1",
    "Points_2",
    "Points_Magnitude",
    "RTData",
    "FieldData: ID",
    "FieldData: STRUCTURED_DIMENSIONS:0",
    "FieldData: STRUCTURED_DIMENSIONS:1",
    "FieldData: STRUCTURED_DIMENSIONS:2"
]

# Open CSV file written by the XML test and verify the column names.
test_output_dir = args.T
test_name = args.N
csv_file_name = test_output_dir + os.sep + test_name + '.csv'
with open(csv_file_name, 'r') as f:
    header = f.readline().strip()

    header_columns = header.split(',')

    if len(expected_columns) != len(header_columns):
        print("Number of columns in '%s' does not match expected number of columns (%d != %d)"
              % (csv_file_name, len(header_columns), len(expected_columns)))
        sys.exit(1)

    for (expected, read) in zip(expected_columns, header_columns):
        if expected != read:
            print("Expected column '%s', read column '%s'" % (expected, read))
            sys.exit(1)

print("%s script passed" % (sys.argv[1]))
