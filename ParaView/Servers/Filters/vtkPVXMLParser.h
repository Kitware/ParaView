/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXMLParser parses ParaView XML configuration files.
// .SECTION Description
// This is a subclass of vtkXMLParser that constructs a representation
// of parsed XML using vtkPVXMLElement.
#ifndef __vtkPVXMLParser_h
#define __vtkPVXMLParser_h

#include "vtkXMLParser.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkPVXMLParser : public vtkXMLParser
{
public:
  vtkTypeRevisionMacro(vtkPVXMLParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLParser* New();

  // Description:
  // Write the parsed XML into the output stream.
  void PrintXML(ostream& os);

  // Description:
  // Get the root element from the XML document.
  vtkPVXMLElement* GetRootElement();

  // Description:
  // Get/Set the file from which to read the configuration.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPVXMLParser();
  ~vtkPVXMLParser();

  void StartElement(const char* name, const char** atts);
  void EndElement(const char* name);
  void CharacterDataHandler(const char* data, int length);

  void AddElement(vtkPVXMLElement* element);
  void PushOpenElement(vtkPVXMLElement* element);
  vtkPVXMLElement* PopOpenElement();

  // The root XML element.
  vtkPVXMLElement* RootElement;

  // The stack of elements currently being parsed.
  vtkPVXMLElement** OpenElements;
  unsigned int NumberOfOpenElements;
  unsigned int OpenElementsSize;

  // Counter to assign unique element ids to those that don't have any.
  unsigned int ElementIdIndex;

  // Called by Parse() to read the stream and call ParseBuffer.  Can
  // be replaced by subclasses to change how input is read.
  virtual int ParseXML();

private:
  vtkPVXMLParser(const vtkPVXMLParser&);  // Not implemented.
  void operator=(const vtkPVXMLParser&);  // Not implemented.
};

#endif
