/*=========================================================================

  Module:    vtkXMLVolumePropertyWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLVolumePropertyWriter - vtkVolumeProperty XML Writer.
// .SECTION Description
// vtkXMLVolumePropertyWriter provides XML writing functionality to 
// vtkVolumeProperty.
// .SECTION See Also
// vtkXMLVolumePropertyReader

#ifndef __vtkXMLVolumePropertyWriter_h
#define __vtkXMLVolumePropertyWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLVolumePropertyWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLVolumePropertyWriter* New();
  vtkTypeRevisionMacro(vtkXMLVolumePropertyWriter,vtkXMLObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the component element used inside that tree to
  // store a component.
  static char* GetComponentElementName();
  static char* GetGrayTransferFunctionElementName();
  static char* GetRGBTransferFunctionElementName();
  static char* GetScalarOpacityElementName();
  static char* GetGradientOpacityElementName();

  // Description:
  // Output part of the object selectively
  vtkBooleanMacro(OutputShadingOnly, int);
  vtkGetMacro(OutputShadingOnly, int);
  vtkSetMacro(OutputShadingOnly, int);

  // Description:
  // Output a given number of components only
  vtkSetClampMacro(NumberOfComponents, int, 1, VTK_MAX_VRCOMP);
  vtkGetMacro(NumberOfComponents, int);

protected:
  vtkXMLVolumePropertyWriter();
  ~vtkXMLVolumePropertyWriter() {};  
  
  int OutputShadingOnly;
  int NumberOfComponents;

  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLVolumePropertyWriter(const vtkXMLVolumePropertyWriter&);  // Not implemented.
  void operator=(const vtkXMLVolumePropertyWriter&);  // Not implemented.
};

#endif


