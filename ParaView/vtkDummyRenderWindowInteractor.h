/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyRenderWindowInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkDummyRenderWindowInteractor - For using styles
// .SECTION Description
// Dummy interactor should not get events.

#ifndef __vtkDummyRenderWindowInteractor_h
#define __vtkDummyRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

class vtkKWApplication;

class VTK_EXPORT vtkDummyRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkDummyRenderWindowInteractor* New();
  vtkTypeMacro(vtkDummyRenderWindowInteractor,vtkRenderWindowInteractor);


protected:
  vtkDummyRenderWindowInteractor();
  ~vtkDummyRenderWindowInteractor();
  vtkDummyRenderWindowInteractor(const vtkDummyRenderWindowInteractor&) {};
  void operator=(const vtkDummyRenderWindowInteractor&) {};

};


#endif


