import argparse
import json
import os
import sys
import zipfile

parser = argparse.ArgumentParser(description='Verify AnimatedExportScene output.')
parser.add_argument('-T', help='Test output directory')
parser.add_argument('-N', help='Test name')
args = parser.parse_args(sys.argv[1:])

# Open the vtkjs archive written by the XML test
test_output_dir = args.T
test_name = args.N
archive_file_name = test_output_dir + os.sep + test_name + '.vtkjs'

with zipfile.ZipFile(archive_file_name, mode='r') as archive:

  assert("index.json" in archive.namelist())

  with archive.open("index.json", mode='r') as index:
    indexStr = index.read()
    indexObj = json.loads(indexStr)

    # Check if we have everything for a basic scene
    assert("version" in indexObj)
    assert("background" in indexObj)
    assert("lookupTables" in indexObj)
    assert("centerOfRotation" in indexObj)
    assert("scene" in indexObj)
    assert("camera" in indexObj)
    assert("focalPoint" in indexObj["camera"])
    assert("position" in indexObj["camera"])
    assert("viewUp" in indexObj["camera"])

    # Check if scene is correct
    assert(len(indexObj["scene"]) == 1)
    source = indexObj["scene"][0]
    sourceType = source["type"]
    assert(sourceType == "vtkHttpDataSetSeriesReader")
    assert("actor" in source)
    assert("actorRotation" in source)
    assert("mapper" in source)
    assert("property" in source)

    # Check that animation is correct
    assert("animation" in indexObj)
    assert(indexObj["animation"]["type"] == "vtkTimeStepBasedAnimationHandler")
    assert(len(indexObj["animation"]["timeSteps"]) == 10)
    for step in indexObj["animation"]["timeSteps"]:
      assert("time" in step)

    # Check if the folder for the source is here and correct
    url = source[sourceType]["url"] + "/"
    assert(url + "index.json" in archive.namelist())
    with archive.open(url + "index.json", mode='r') as sourceIndex:
      sourceIndexObj = json.loads(sourceIndex.read())
      assert(len(sourceIndexObj["series"]) == 10)
      for step in sourceIndexObj["series"]:
        assert("timeStep" in step)
        indexStepPath = url + step["url"] + "/index.json"
        assert(indexStepPath in archive.namelist())

    # Check that there's only 26 data array
    assert(sum(map(lambda x : x.startswith("data/"), archive.namelist())) == 26)
