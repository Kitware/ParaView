/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightReaderInterface.h
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
// .NAME vtkPVEnSightReaderInterface - Everything PV needs to make a UI for a filter.
// .SECTION Description
// This contains every thing PV needs to create a UI for a VTK source.

#ifndef __vtkPVEnSightReaderInterface_h
#define __vtkPVEnSightReaderInterface_h

#include "vtkPVSourceInterface.h"

class VTK_EXPORT vtkPVEnSightReaderInterface : public vtkPVSourceInterface
{
public:
  static vtkPVEnSightReaderInterface* New();
  vtkTypeMacro(vtkPVEnSightReaderInterface, vtkPVSourceInterface);

  vtkPVSource* CreateCallback();

  void SaveInTclScript(ofstream *file, const char *sourceName);
  
protected:
  vtkPVEnSightReaderInterface();
  ~vtkPVEnSightReaderInterface() {};
  vtkPVEnSightReaderInterface(const vtkPVEnSightReaderInterface&) {};
  void operator=(const vtkPVEnSightReaderInterface&) {};

  vtkSetStringMacro(CaseFileName);
  char* CaseFileName;
};

#endif
