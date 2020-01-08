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
/**
 * @class   vtkSelectionSerializer
 * @brief   Serialize/deserialize vtkSelection to/from xml
 *
 * vtkSelectionSerializer is a helper class that can
 * serialize/deserialize vtkSelection to/from xml. Currently, it
 * supports only a subset of properties: CONTENT_TYPE, SOURCE_ID,
 * PROP_ID, PROCESS_ID, ORIGINAL_SOURCE_ID
 * @sa
 * vtkSelection
*/

#ifndef vtkSelectionSerializer_h
#define vtkSelectionSerializer_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

class vtkInformationIntegerKey;
class vtkPVXMLElement;
class vtkSelection;
class vtkSelectionNode;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkSelectionSerializer : public vtkObject
{
public:
  static vtkSelectionSerializer* New();
  vtkTypeMacro(vtkSelectionSerializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Serialize the selection tree to a stream as xml.
   * For now, only keys of type vtkInformationIntegerKey are supported.
   */
  static void PrintXML(int printData, vtkSelection* selection);
  static void PrintXML(ostream& os, vtkIndent indent, int printData, vtkSelection* selection);
  //@}

  //@{
  /**
   * Parse an xml string to create a new selection tree.
   * The string is 0 terminated for the first version of this function,
   * or we specify the length of the string for the second version.
   * Currently, this supports only a subset of
   * properties: CONTENT_TYPE, SOURCE_ID, PROP_ID, PROCESS_ID
   */
  static void Parse(const char* xml, vtkSelection* root);
  static void Parse(const char* xml, unsigned int length, vtkSelection* root);
  //@}

  /**
   * ID of the dataset or algorithm that the selection belongs to. What
   * ID means is application specific.
   */
  static vtkInformationIntegerKey* ORIGINAL_SOURCE_ID();

protected:
  vtkSelectionSerializer();
  ~vtkSelectionSerializer() override;

private:
  vtkSelectionSerializer(const vtkSelectionSerializer&) = delete;
  void operator=(const vtkSelectionSerializer&) = delete;

  static void WriteSelectionData(ostream& os, vtkIndent indent, vtkSelectionNode* selection);
  static void Parse(vtkPVXMLElement* rootElem, vtkSelection* root);
  static void ParseNode(vtkPVXMLElement* nodeXML, vtkSelectionNode* node);
};

#endif
