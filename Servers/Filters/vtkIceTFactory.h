/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTFactory.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTFactory
// .SECTION Description
// A factory for automagically loading the stuff needed to do parallel
// ICE-T compositing.  Most importantly, it makes vtkIceTRenderer the
// default vtkRenderer.  It also makes vtkIceTComposite the default
// vtkCompositeManager; however, this will probablly prove to be of little
// use.
// .SECTION note
// This class is built even if the ICE-T classes are not.  If this is the
// case, the factory object does not do anything useful, but does nothing
// harmful either.
// .SECTION see also
// vtkIceTComposite

#ifndef __vtkIceTFactory_h
#define __vtkIceTFactory_h

#include "vtkObjectFactory.h"

class VTK_EXPORT vtkIceTFactory : public vtkObjectFactory
{
public:
  vtkTypeRevisionMacro(vtkIceTFactory, vtkObjectFactory);
  static vtkIceTFactory *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual const char *GetVTKSourceVersion();
  virtual const char *GetDescription();

protected:
  vtkIceTFactory();
  virtual ~vtkIceTFactory() { }

private:
  vtkIceTFactory(const vtkIceTFactory &); // Not implemented.
  void operator=(const vtkIceTFactory &); // Not implemented.
};

#endif //__vtkIceTFactory_h
