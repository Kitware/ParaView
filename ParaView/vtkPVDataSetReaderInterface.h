/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetReaderInterface.h
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
// .NAME vtkPVDataSetReaderInterface - Interface for readers with unresolved outputs.
// .SECTION Description
// Some readers can generate any type of output based on the file read.
// This interface prompts the user with a dialog before the reader is created.
// It then creates vtkPVSources which are just dummies holding the outputs.
// This interface can also handle multiple outputs. 

// This Interface is almost identical to the vtkEnSightreaderInterface.  
// They could be merged. (Only difference FileName vs CaseFileName).

#ifndef __vtkPVDataSetReaderInterface_h
#define __vtkPVDataSetReaderInterface_h

#include "vtkPVSourceInterface.h"

class VTK_EXPORT vtkPVDataSetReaderInterface : public vtkPVSourceInterface
{
public:
  static vtkPVDataSetReaderInterface* New();
  vtkTypeMacro(vtkPVDataSetReaderInterface, vtkPVSourceInterface);

  vtkPVSource* CreateCallback();
  
  void SaveInTclScript(ofstream *file, const char *sourceName);
  
protected:
  vtkPVDataSetReaderInterface();
  ~vtkPVDataSetReaderInterface() {};
  vtkPVDataSetReaderInterface(const vtkPVDataSetReaderInterface&) {};
  void operator=(const vtkPVDataSetReaderInterface&) {};
  
  // necessary for writing out pipeline
  vtkSetStringMacro(FileName);
  char* FileName;
};

#endif
