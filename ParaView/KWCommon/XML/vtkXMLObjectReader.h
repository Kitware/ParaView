/*=========================================================================

  Module:    vtkXMLObjectReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLObjectReader - XML Object Reader.
// .SECTION Description
// vtkXMLObjectReader provides base functionalities for all XML readers.
// .SECTION See Also
// vtkXMLObjectWriter

#ifndef __vtkXMLObjectReader_h
#define __vtkXMLObjectReader_h

#include "vtkXMLIOBase.h"

class vtkXMLDataElement;
class vtkXMLDataParser;

class VTK_EXPORT vtkXMLObjectReader : public vtkXMLIOBase
{
public:
  vtkTypeRevisionMacro(vtkXMLObjectReader,vtkXMLIOBase);

  // Description:
  // Parse an XML tree and modify the object accordingly.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Parse an XML stream.
  // Return 1 on success, 0 on error.
  virtual int ParseStream(istream&);

  // Description:
  // Parse an XML string.
  // Return 1 on success, 0 on error.
  virtual int ParseString(const char*);

  // Description:
  // Parse an XML file.
  // Return 1 on success, 0 on error.
  virtual int ParseFile(const char*);

  // Description:
  // Convenience method to look in 'parent' for an XML element matching 
  // the current root name and parse that element accordingly (see Parse()).
  // IsInElement() does not actually parse, but checks if the
  // element can be found on the path described previously.
  // Return 1 on success, 0 on error.
  virtual int ParseInElement(vtkXMLDataElement *parent);
  virtual int IsInElement(vtkXMLDataElement *parent);

  // Description:
  // Convenience method to look in 'grandparent' for an XML 'parent' element
  // matching the 'name', then look inside 'parent' for an XML element matching
  // the current root name and parse that element accordingly 
  // (see ParseInElement()).
  // IsInNestedElement() does not actually parse, but checks if the
  // element can be found on the path described previously.
  // Return 1 on success, 0 on error.
  virtual int ParseInNestedElement(vtkXMLDataElement *grandparent,
                                   const char *name);
  virtual int IsInNestedElement(vtkXMLDataElement *grandparent,
                                const char *name);

protected:
  vtkXMLObjectReader();
  ~vtkXMLObjectReader();  

  // Description:
  // Create and destroy the XML parser.
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  
  // Description:
  // InitializeParsing is called before each call to Parse(element).
  virtual int InitializeParsing();

  // A vtkXMLDataParser instance used to hide XML reading details
  // and create an XML tree from an XML stream.

  vtkXMLDataParser* XMLParser;

  // Store the element that was parsed by Parse()

  vtkXMLDataElement *LastParsedElement;

private:
  vtkXMLObjectReader(const vtkXMLObjectReader&);  // Not implemented.
  void operator=(const vtkXMLObjectReader&);  // Not implemented.
};

#endif


