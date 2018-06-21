#!/usr/bin/python3

# This script takes as input a json file assumed to have a list as its
# root element and parses it, removing duplicate elements in the list.
# The new list of unique elements is written out to a given output file.

# Used to prune duplicates from the ParaView color map list
if __name__ == '__main__':
    import sys
    import os.path

    # get icon dir
    if len(sys.argv) < 3:
        print('Usage: %s <input_json> <output_json>' % sys.argv[0])
        exit(1)

    input_json = sys.argv[1]
    output_file = sys.argv[2]
    if not os.path.exists(input_json):
        print('Input file %s does not exist!' % input_json)
        exit(2)

    import json
    newcmaps = list()
    with open(input_json, 'r') as f:
        cmaps = json.load(f)
        for i in range(len(cmaps)):
            cm = cmaps[i]
            if cm not in newcmaps:
                newcmaps.append(cm)
            else:
                print("Found duplicate colormap \"%s\"" % cm["Name"])

    with open(output_file, 'w') as f:
        json.dump(newcmaps, f, indent=2, separators=(',', ': '))
