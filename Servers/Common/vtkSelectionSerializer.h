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
// vtkSelectionSerializer is a helper class that can
// serialize/deserialize vtkSelection to/from xml. Currently, it
// supports only a subset of properties: CONTENT_TYPE, SOURCE_ID,
// PROP_ID, PROCESS_ID, ORIGINAL_SOURCE_ID
// .SECTION See Also
// vtkSelection

#ifndef __vtkSelectionSerializer_h
#define __vtkSelectionSerializer_h

#include "vtkObject.h"

class vtkInformationIntegerKey;
class vtkPVXMLElement;
class vtkSelection;
class vtkSelectionNode;

class VTK_EXPORT vtkSelectionSerializer : public vtkObject
{
public:
  static vtkSelectionSerializer* New();
  vtkTypeMacro(vtkSelectionSerializer,vtkObject);
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

  // Description:
  // ID of the dataset or algorithm that the selection belongs to. What
  // ID means is application specific.
  static vtkInformationIntegerKey* ORIGINAL_SOURCE_ID();

protected:
  vtkSelectionSerializer();
  ~vtkSelectionSerializer();

private:
  vtkSelectionSerializer(const vtkSelectionSerializer&);  // Not implemented.
  void operator=(const vtkSelectionSerializer&);  // Not implemented.

  static void WriteSelectionData(ostream& os, 
                                 vtkIndent indent, 
                                 vtkSelectionNode* selection);
  static void ParseNode(
    vtkPVXMLElement* nodeXML, vtkSelectionNode* node);
};

#endif
