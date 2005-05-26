/*=========================================================================

  Module:    vtkKWSaveImageDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSaveImageDialog - a dialog for saving 2D images
// .SECTION Description
// A simple dialog for saving a 2D image as a BMP, TIFF, or PNM.

#ifndef __vtkKWSaveImageDialog_h
#define __vtkKWSaveImageDialog_h

#include "vtkKWLoadSaveDialog.h"
class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWSaveImageDialog : public vtkKWLoadSaveDialog
{
public:
  static vtkKWSaveImageDialog* New();
  vtkTypeRevisionMacro(vtkKWSaveImageDialog,vtkKWLoadSaveDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description::
  // Invoke the dialog 
  virtual int Invoke();

protected:
  vtkKWSaveImageDialog();
  ~vtkKWSaveImageDialog() {};

private:
  vtkKWSaveImageDialog(const vtkKWSaveImageDialog&); // Not implemented
  void operator=(const vtkKWSaveImageDialog&); // Not implemented
};


#endif
