/*=========================================================================

  Module:    vtkKWGenericComposite.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWGenericComposite - a simple composite with one prop
// .SECTION Description
// vtkKWGenericComposite is a subclass of vtkKWComposite designed to 
// simply contain one prop. 

#ifndef __vtkKWGenericComposite_h
#define __vtkKWGenericComposite_h


#include "vtkKWComposite.h"

class vtkKWApplication;
class vtkKWView;
class vtkStructuredPoints;
class vtkProp;

class VTK_EXPORT vtkKWGenericComposite : public vtkKWComposite
{
public:
  static vtkKWGenericComposite* New();
  vtkTypeRevisionMacro(vtkKWGenericComposite,vtkKWComposite);
  void PrintSelf(ostream& os, vtkIndent indent);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define SetPropA SetProp
# define SetPropW SetProp
#endif

  //BTX
  // Description:
  // Set the prop for this composite.
  void SetProp(vtkProp*);
  //ETX

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef SetPropW
# undef SetPropA
  //BTX
  // Define possible mangled names.
  void SetPropA(vtkProp*);
  void SetPropW(vtkProp*);
  //ETX
#endif

protected:
  vtkKWGenericComposite();
  ~vtkKWGenericComposite();

  vtkProp *Prop;

  // Define method required by superclass.
  virtual vtkProp* GetPropInternal();

private:
  vtkKWGenericComposite(const vtkKWGenericComposite&); // Not implemented
  void operator=(const vtkKWGenericComposite&); // Not implemented
};


#endif



