/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScaleFactorEntry.h
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
// .NAME vtkPVScaleFactorEntry - entry specifically for scale factors
// .SECTION Description
// vtkPVScaleFactorEntry is a subclass of vtkPVVectorEntry that depends
// on a vtkPVInputMenu to determine what its default scale value should be.

#ifndef __vtkPVScaleFactorEntry_h
#define __vtkPVScaleFactorEntry_h

#include "vtkPVVectorEntry.h"

class vtkPVInputMenu;

class VTK_EXPORT vtkPVScaleFactorEntry : public vtkPVVectorEntry
{
public:
  static vtkPVScaleFactorEntry* New();
  vtkTypeRevisionMacro(vtkPVScaleFactorEntry, vtkPVVectorEntry);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

  // Description:
  // Move widget state to vtk object or back.
  virtual void ResetInternal();
  virtual void AcceptInternal(const char* sourceTclName);
  
protected:
  vtkPVScaleFactorEntry();
  ~vtkPVScaleFactorEntry();
  
  void UpdateScaleFactor();

//BTX
  virtual void CopyProperties(vtkPVWidget *clone, vtkPVSource *pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);  
  
  vtkPVInputMenu *InputMenu;
  vtkPVSource *Input;
  void SetInput(vtkPVSource *input);
  int AcceptCalled;
  
private:
  vtkPVScaleFactorEntry(const vtkPVScaleFactorEntry&); // Not implemented
  void operator=(const vtkPVScaleFactorEntry&); // Not implemented
};

#endif
