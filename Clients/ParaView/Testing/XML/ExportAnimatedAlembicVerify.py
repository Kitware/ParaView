import os
import sys
import argparse

from pathlib import Path

parser = argparse.ArgumentParser(description='Verify ExportAnimatedAlembic output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

expected_frames = [5, 7, 9, 11, 13, 15]

# Open the Alembic files written by the XML test and verify they look like
# valid Ogawa archives. Deep content verification would require the Alembic
# Python bindings, which aren't expected to be available, so this only
# sanity-checks that the file series was written with distinct per-frame data.
test_output_dir = args.T
test_name = args.N
abc_root_file_name = test_output_dir + os.sep + test_name

contents_by_frame = {}

for timestep in expected_frames:
    abc_file_name = f"{abc_root_file_name}.{str(timestep)}.abc"
    file_path = Path(abc_file_name)
    if not file_path.is_file():
        print(f"Error: missing expected output file: {str(file_path)}")
        sys.exit(1)

    with open(file_path, 'rb') as f:
        content = f.read()
    if not content.startswith(b'Ogawa'):
        print(f"Error: {file_path} does not look like a valid Ogawa Alembic file, got header: {content[:5]!r}")
        sys.exit(1)

    # The exporter does not optimize for reusing the same color texture
    # across timesteps, so each frame's Alembic file should reference a
    # unique color texture file. Check that the expected color texture
    # file exists.
    texture_file_name = f"{abc_root_file_name}.{str(timestep)}_tex0.png"
    texture_path = Path(texture_file_name)
    if not texture_path.is_file():
        print(f"Error: missing expected color texture file: {str(texture_path)}")
        sys.exit(1)

    contents_by_frame[timestep] = content

# Since each frame is colored by a different harmonics value, the data exported
# for each frame should be different.
unique_contents = set(contents_by_frame.values())
if len(unique_contents) != len(contents_by_frame):
    print("Error: exported Alembic files are not unique across timesteps")
    sys.exit(1)

print(f"{test_name} script passed")
