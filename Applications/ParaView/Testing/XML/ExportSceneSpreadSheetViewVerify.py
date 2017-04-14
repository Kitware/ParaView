import os
import sys
import argparse

parser = argparse.ArgumentParser(description='Verify ExportSceneSpreadSheetView output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

expected_columns = [
    '"Time"',
    '"avg ACCL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"min ACCL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"max ACCL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q1 ACCL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q3 ACCL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"avg DISPL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"min DISPL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"max DISPL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q1 DISPL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q3 DISPL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"avg VEL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"min VEL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"max VEL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q1 VEL (Magnitude) (Block: 2 ; Point Statistics)"',
    '"q3 VEL (Magnitude) (Block: 2 ; Point Statistics)"'
    ]

# Open CSV file written by the XML test and verify the column names.
test_output_dir = args.T
test_name = args.N
csv_file_name = test_output_dir + os.sep + test_name + '.csv'
with open(csv_file_name, 'r') as f:
    header = f.readline().strip()

    header_columns = header.split(',')

    if len(expected_columns) != len(header_columns):
        print("Number of columns in '%s' does not match expected number of colums"
              % csv_file_name)
        sys.exit(1)

    for (expected, read) in zip(expected_columns, header_columns):
        if expected != read:
            print("Expected column '%s', read column '%s'" % (expected, read))
            sys.exit(1)

print("%s script passed" % (sys.argv[1]))
