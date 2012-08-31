#script to resize openoffice sized images to something readable
import re
import sys
fname = sys.argv[1:]
match_image = re.compile(".*WIDTH=(\d+) HEIGHT=(\d+).*")
fi = open(fname[0], 'r')
for line in fi:
    match = match_image.match(line)
    if match != None:
        width = int(match.group(1))
        width2 = int(width*2.5)
        height = int(match.group(2))
        height2 = int(height*2.5)
        print re.sub(str(width), str(width2), re.sub(str(height), str(height2), line)),
    else:
	print line,
