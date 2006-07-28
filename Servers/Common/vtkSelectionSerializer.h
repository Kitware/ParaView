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
// vtkSelection vtkSelectionNode

#ifndef __vtkSelectionSerializer_h
#define __vtkSelectionSerializer_h

#include "vtkObject.h"

class vtkPVXMLElement;
class vtkSelection;
class vtkSelectionNode;

class VTK_EXPORT vtkSelectionSerializer : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSelectionSerializer,vtkObject);

  // Description:
  // Serialize the selection tree to a stream as xml.
  // For now, only keys of type vtkInformationIntegerKey are supported.
  static void PrintXML(ostream& os, 
                       vtkIndent indent, 
                       int printData, 
                       vtkSelectionNode* selection);

  // Description:
  // Parse an xml string to create a new selection tree. It is
  // the caller's responsibility to delete the returned object.
  // Currently, this supports only a subset of
  // properties: CONTENT_TYPE, SOURCE_ID, PROP_ID, PROCESS_ID
  static vtkSelection* Parse(const char* xml);


protected:
  vtkSelectionSerializer();
  ~vtkSelectionSerializer();

private:
  vtkSelectionSerializer(const vtkSelectionSerializer&);  // Not implemented.
  void operator=(const vtkSelectionSerializer&);  // Not implemented.

  static void WriteSelectionList(ostream& os, 
                                 vtkIndent indent, 
                                 vtkSelectionNode* selection);
  static void ParseNode(
    vtkPVXMLElement* nodeXML, vtkSelectionNode* node);
};

#endif
