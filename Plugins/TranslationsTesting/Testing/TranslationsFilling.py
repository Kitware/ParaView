# This script takes as input Qt translations
# source files and modify them to fill them
# with translations made of "_TranslationTesting"
import xml.etree.ElementTree as ET
import argparse
import os
import re


def replaceByEscapable_TranslationTestings(text: str) -> str:
    """
    Generates '_TranslationTesting expressions', a repetition of n + 1 _TranslationTesting, n being
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
        translation_node = node.find('translation')
        translation_node.attrib.pop("type", None)
        source_node = node.find('source')
        if translation_node is None or source_node is None or source_node.text is None:
            print(f"Error: Message with missing translation or source: {ET.tostring(node, encoding='unicode')}")
            return
        translation_node.text = replaceByEscapable_TranslationTestings(source_node.text)
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
