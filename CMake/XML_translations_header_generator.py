# This script takes as input a Proxy XML file
# and return a cpp header file that Qt Linguist
# tools can itself transform into a Qt translation
# source file.
# For example, this XML file:
"""
<ServerManagerConfiguration>
    <ProxyGroup name="example">
        <SourceProxy label="Image Data To AMR"
                     name="ImageDataToAMR">
        </SourceProxy>
    </ProxyGroup>
</ServerManagerConfiguration>
"""
# would result in the following header:
"""
#include <QtGlobal>
#include <iostream>
#include <string>

static const char *messages[] = {
    //: Real source: example.xml:2 - example.xml
    QT_TRANSLATE_NOOP("ServerManagerXML", R"(example)"),

    //: Real source: example.xml:3 - example
    QT_TRANSLATE_NOOP("ServerManagerXML", R"(Image Data To AMR)")
};
"""

import sys
# Forbid the use of C modules so the parser overload can get the correct line numbers
sys.modules['_elementtree'] = None

import xml.etree.ElementTree as ET
import argparse
from collections.abc import Iterable
import os

def translationUnit(file: str, line: int, context: str, content: str) -> str:
    if not content:
        return ""
    content = ' '.join(content.replace('"', '\\"').split())
    # line is not used for now as it creates git merge conflict when modifying the translation and ParaView at the same time
    res = f"\t//: Real source: {file} - {context}\n"
    res += f"\tQT_TRANSLATE_NOOP(\"ServerManagerXML\", R\"({content})\"),\n\n"
    return res

def _insertSpace(string: str, index: int) -> str:
    return string[:index] + ' ' + string[index:]

# This function MUST have the same behavior as
# vtkSMObject::CreatePrettyLabel()
def createPrettyLabel(label: str) -> str:
    """
    Create a proper label from a name.
    """
    if label == "":
        return ""
    label = " " + label + "  "
    for i in range(0, len(label)):
        if label[i - 1].isspace() or label[i + 1].isspace():
            continue
        if label[i].isupper() and label[i + 1].islower():
            label = _insertSpace(label, i)
        if label[i].islower() and label[i + 1].isupper():
            label = _insertSpace(label, i + 1)
    return label[1:-2]

def recursiveStringCrawl(file: str, context: str, group: str, node) -> list:
    """
    Recursive function searching for node labels to translate.
    Return a list of its translatable attributes (and from its children)
    to translate as Qt header format.
    """

    res = []

    # Identify proxy group
    if node.tag == "ProxyGroup":
      group = node.attrib["name"]

    else:
      if "label" in node.attrib:
          res.append(translationUnit(file, node.line, context, node.attrib["label"]))
      elif "menu_label" in node.attrib:
          res.append(translationUnit(file, node.line, context, node.attrib["menu_label"]))
      elif "Documentation" in node.tag:
          if node.text:
              res.append(translationUnit(file, node.line, context, node.text))
          if "long_help" in node.attrib:
              res.append(translationUnit(file, node.line, context, node.attrib["long_help"]))
          if "short_help" in node.attrib:
              res.append(translationUnit(file, node.line, context, node.attrib["short_help"]))
      elif "Text" in node.tag:
          if node.text:
              res.append(translationUnit(file, node.line, context, node.text))
          if "title" in node.attrib:
              res.append(translationUnit(file, node.line, context, node.attrib["title"]))
      elif "Property" in node.tag and "name" in node.attrib:
          res.append(translationUnit(file, node.line, context, createPrettyLabel(node.attrib["name"])))
      elif node.tag.endswith("Proxy") and not group.startswith("internal_") and "name" in node.attrib:
          res.append(translationUnit(file, node.line, context, createPrettyLabel(node.attrib["name"])))
      elif "ShowInMenu" in node.tag and "category" in node.attrib:
          res.append(translationUnit(file, node.line, context, node.attrib["category"]))

      if node.tag.endswith("Proxy") and "name" in node.attrib:
          context = node.attrib["name"]

    for child in node:
        res += recursiveStringCrawl(file, context, group, child)

    return res

class LineParser(ET.XMLParser):
    """
    Overload default XML parser to give nodes a line attribute.
    """
    def _start(self, *args, **kwargs):
        el = super(self.__class__, self)._start(*args, **kwargs)
        el.line = self.parser.CurrentLineNumber
        return el

def removePrefix(string: str, prefix: str):
    if string.startswith(prefix):
        return string[len(prefix):]
    return string

def fileParsing(fileIn: str, sourceDir: str) -> str:
    """
    Starts recursiveStringCrawl at xml file root.
    Returns the string of all labels to translate as
    Qt header format.
    """
    xmlTree = ET.parse(fileIn, parser=LineParser())
    stringsList = recursiveStringCrawl(removePrefix(fileIn, sourceDir), removePrefix(fileIn, sourceDir), "root", xmlTree.getroot())
    res = "".join(stringsList)
    return res

def filesManager(filesIn: list, fileOut: str, sourceDir: str):
    """
    Create Qt header first lines then call
    fileParsing for each file and appends them
    to the result file.
    """
    if not isinstance(filesIn, Iterable):
        raise ValueError("Not an array")
    res = "#include <QtGlobal>\n#include <iostream>\n#include <string>\n\n"
    res += "static const char *messages[] = {\n"
    for file in filesIn:
        res += fileParsing(file, sourceDir)
    # Removing a comma and newline after last element of the "message" list.
    res = res[:-2]
    res += "\n};\n"
    with open(fileOut, 'w', encoding='utf-8') as f:
        f.write(res)

def __main__():
    """
    Asks for an ouput file and for one or more input files.
    """
    parser = argparse.ArgumentParser("Generates C++ headers with XML content to be translated")
    parser.add_argument('-o', '--out', help='Result header filename', nargs="?")
    parser.add_argument('-s', '--source', help='Client source directory path', nargs=1)
    parser.add_argument('inFiles', metavar='N', nargs="+")
    args = parser.parse_args()
    for i in args.inFiles:
        if not os.path.exists(i):
            raise ValueError(f"Invalid input file '{i}'")
    if args.out is not None and args.source is not None:
        filesManager(args.inFiles, args.out, args.source[0])
    else:
        for i in args.inFiles:
            filesManager([i], os.path.splitext(i)[0] + ".h")

if __name__ == "__main__":
    __main__()
