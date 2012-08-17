
import os
import os.path
import sys
import xml.sax.handler
import xml.sax


#---------------------------------------------------------------------------
class ClassExtractor(xml.sax.handler.ContentHandler):
    """This class is essentially a callback. It is used to parse the xml files.
    It searches for "class" attribute and collects it in a 'classes' array"""

    def __init__(self):
        self.classes = []

    def startElement(self, name, attributes):
        if attributes.has_key("class"):
            self.classes.append(attributes["class"])


#---------------------------------------------------------------------------
def parseAndGetUniqueClasses(filesList):
    """Given a list of XML files, parses the files to get a list of
    unique vtk class names for all vtk class names"""
    parser = xml.sax.make_parser()
    handler= ClassExtractor()
    parser.setContentHandler(handler)
    classNames = []
    for item in filesList:
        parser.parse(item)
        for className in handler.classes:
            if len(className) and className != "not-used":
                classNames.append(className)
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
    if len(argv) < 3:
        print "Usage: python ParseXMLResources.py <Output-File-Name> <List-Of-XML-Files>"
        sys.exit(1)

    classList = parseAndGetUniqueClasses(argv[2:])
    writeStrings(classList, argv[1])


#---------------------------------------------------------------------------
if __name__ == "__main__":
    main()

