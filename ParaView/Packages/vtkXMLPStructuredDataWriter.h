/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredDataWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPStructuredDataWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPStructuredDataWriter

#ifndef __vtkXMLPStructuredDataWriter_h
#define __vtkXMLPStructuredDataWriter_h

#include "vtkXMLPDataWriter.h"

class vtkXMLStructuredDataWriter;

class VTK_EXPORT vtkXMLPStructuredDataWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLPStructuredDataWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the extent translator used for creating pieces.
  virtual void SetExtentTranslator(vtkExtentTranslator*);
  vtkGetObjectMacro(ExtentTranslator, vtkExtentTranslator);
  
protected:
  vtkXMLPStructuredDataWriter();
  ~vtkXMLPStructuredDataWriter();
  
  virtual vtkXMLStructuredDataWriter* CreateStructuredPieceWriter()=0;
  void WritePrimaryElementAttributes();
  void WritePPieceAttributes(int index);
  vtkXMLWriter* CreatePieceWriter();
  
  vtkExtentTranslator* ExtentTranslator;
  
private:
  vtkXMLPStructuredDataWriter(const vtkXMLPStructuredDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPStructuredDataWriter&);  // Not implemented.
};

#endif
