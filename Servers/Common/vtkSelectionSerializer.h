/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionSerializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionSerializer - Serialize/deserialize vtkSelection to/from xml
// .SECTION Description
// vtkSelectionSerializer is a helper class that can serialize/deserialize
// vtkSelection to/from xml. Currently, it supports only a subset of
// properties: CONTENT_TYPE, SOURCE_ID, PROP_ID, PROCESS_ID
// .SECTION See Also
// vtkSelection

#ifndef __vtkSelectionSerializer_h
#define __vtkSelectionSerializer_h

#include "vtkObject.h"

class vtkPVXMLElement;
class vtkSelection;
class vtkSelection;

class VTK_EXPORT vtkSelectionSerializer : public vtkObject
{
public:
  static vtkSelectionSerializer* New();
  vtkTypeRevisionMacro(vtkSelectionSerializer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Serialize the selection tree to a stream as xml.
  // For now, only keys of type vtkInformationIntegerKey are supported.
  static void PrintXML(int printData, 
                       vtkSelection* selection);
  static void PrintXML(ostream& os, 
                       vtkIndent indent, 
                       int printData, 
                       vtkSelection* selection);

  // Description:
  // Parse an xml string to create a new selection tree.
  // Currently, this supports only a subset of
  // properties: CONTENT_TYPE, SOURCE_ID, PROP_ID, PROCESS_ID
  static void Parse(const char* xml, vtkSelection* root);


protected:
  vtkSelectionSerializer();
  ~vtkSelectionSerializer();

private:
  vtkSelectionSerializer(const vtkSelectionSerializer&);  // Not implemented.
  void operator=(const vtkSelectionSerializer&);  // Not implemented.

  static void WriteSelectionList(ostream& os, 
                                 vtkIndent indent, 
                                 vtkSelection* selection);
  static void ParseNode(
    vtkPVXMLElement* nodeXML, vtkSelection* node);
};

#endif
