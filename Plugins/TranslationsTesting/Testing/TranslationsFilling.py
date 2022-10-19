# This script takes as input Qt translations
# source files and modify them to fill them
# with translations made of "_TranslationTesting"
import xml.etree.ElementTree as ET
import argparse
import os
import re


def replaceByEscapable_TranslationTestings(text: str) -> str:
    """
    Generates '_TranslationTesting expressions', a repetion of n + 1 _TranslationTesting, n being
    the number of Qt escape sequences (%1, %2...)
    """
    res = "_TranslationTesting" + "".join(re.findall("%[0-9]{1,2}", text))
    if "&" in text:
        res = "&" + res
    return res


def recTranslations(node):
    """
    Recursively replaces all unfinished translations in the tree nodes by _TranslationTesting expressions
    from `replaceByEscapable_TranslationTestings`.
    """
    if node.tag == "message":
        node.find('translation').attrib.pop("type", None)
        node.find('translation').text = replaceByEscapable_TranslationTestings(node.find('source').text)
    else:
        for child in node:
            recTranslations(child)


def fileParsing(fileIn: str):
    """
    Parses the XML file, then call `recTranslations` on the
    root before saving the modified tree.
    """
    xmlTree = ET.parse(fileIn)
    recTranslations(xmlTree.getroot())
    xmlTree.write(fileIn)


def __main__():
    """
    Asks for one or more input files to edit.
    """
    parser = argparse.ArgumentParser("Rewrites all translations in a ts by '_TranslationTesting'")
    parser.add_argument('inFiles', metavar='N', nargs="+")
    args = parser.parse_args()
    for i in args.inFiles:
        if not os.path.exists(i):
            raise ValueError(f"Invalid input file '{i}'")
    for i in args.inFiles:
        fileParsing(i)


if __name__ == "__main__":
    __main__()
