/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLActor2DWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkXMLActor2DWriter - vtkActor2D XML Writer.
// .SECTION Description
// vtkXMLActor2DWriter provides XML writing functionality to 
// vtkActor2D.
// .SECTION See Also
// vtkXMLActor2DReader

#ifndef __vtkXMLActor2DWriter_h
#define __vtkXMLActor2DWriter_h

#include "vtkXMLPropWriter.h"

class VTK_EXPORT vtkXMLActor2DWriter : public vtkXMLPropWriter
{
public:
  static vtkXMLActor2DWriter* New();
  vtkTypeRevisionMacro(vtkXMLActor2DWriter,vtkXMLPropWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the property element used inside that tree to
  // store a property.
  static char* GetPropertyElementName();

protected:
  vtkXMLActor2DWriter() {};
  ~vtkXMLActor2DWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLActor2DWriter(const vtkXMLActor2DWriter&);  // Not implemented.
  void operator=(const vtkXMLActor2DWriter&);  // Not implemented.
};

#endif
