import os
import sys
import argparse

from pathlib import Path

parser = argparse.ArgumentParser(description='Verify ExportAnimatedUSD output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

expected_frames = [5, 7, 9, 11, 13, 15]

# Open USD file written by the XML test and verify it looks like a valid
# ASCII USD (.usda) file. Deep content verification would require the `pxr`
# Python bindings, which aren't expected to be available, so this
# only sanity-checks the file series was written.
test_output_dir = args.T
test_name = args.N
usd_root_file_name = test_output_dir + os.sep + test_name

contents_by_frame = {}

for timestep in expected_frames:
    usd_file_name = f"{usd_root_file_name}.{str(timestep)}.usda"
    file_path = Path(usd_file_name)
    if not file_path.is_file():
        print(f"Error: missing expected output file: {str(file_path)}")
        sys.exit(1)

    with open(file_path, 'r') as f:
        content = f.read()
    if not content.startswith('#usda'):
        print(f"Error: {file_path} does not look like a valid USDA file, got header: {content.splitlines()[0]!r}")
        sys.exit(1)
    if 'def Mesh ' not in content:
        print(f"Error: {file_path} does not contain any exported Mesh geometry")
        sys.exit(1)

    # The exporter does not optimize for reusing the same color texture
    # across timesteps, so each frame's USD file should reference a unique
    # color texture file. Check that the expected color texture file exists.
    texture_file_name = f"{usd_root_file_name}.{str(timestep)}_tex0.png"
    texture_path = Path(texture_file_name)
    if not texture_path.is_file():
        print(f"Error: missing expected color texture file: {str(texture_path)}")
        sys.exit(1)

    contents_by_frame[timestep] = content

# Since each frame is colored by a different harmonics value, the data exported
# for each frame should be different.
unique_contents = set(contents_by_frame.values())
if len(unique_contents) != len(contents_by_frame):
    print("Error: exported USD files are not unique across timesteps")
    sys.exit(1)

print(f"{test_name} script passed")
