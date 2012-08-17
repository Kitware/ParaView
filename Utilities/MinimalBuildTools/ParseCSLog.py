import sys
import re


#---------------------------------------------------------------------------
def getUniqueClasses(inFile):

    inFileHandle = open(sys.argv[1], 'r')
    inFileContents = inFileHandle.read()
    inFileHandle.close()

    # look in file for this pattern: {vtkSomeClass}
    rePattern = re.compile('{(vtk\S*).*}')
    matchList = rePattern.findall(inFileContents)

    classNames = []
    for match in matchList:
        classNames.append(match)
    return sortAndRemoveDuplicates(classNames)


#---------------------------------------------------------------------------
def sortAndRemoveDuplicates(stringList):
    d = dict() # using dict instead of set
    for s in stringList: d[s] = None
    uniqueList = d.keys()
    uniqueList.sort()
    return uniqueList


#---------------------------------------------------------------------------
def writeStrings(stringList, outFile):
    """Writes a list of strings to a file, writing one string per line"""
    fd = open(outFile, 'w')
    for string in stringList:
        fd.write(string)
        fd.write("\n")


#---------------------------------------------------------------------------
def printStrings(stringList):
    for string in stringList: print string


#---------------------------------------------------------------------------
def main(argv=None):
    if not argv: argv = sys.argv
    if len(argv) != 3:
        print "Usage: python ParseCSLog.py <log-file> <out-file>"
        sys.exit(1)

    classList = getUniqueClasses(argv[1])
    writeStrings(classList, argv[2])


#---------------------------------------------------------------------------
if __name__ == "__main__":
    main()

