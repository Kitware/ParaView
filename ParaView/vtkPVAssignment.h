/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAssignment.h
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
// .NAME vtkKWAssignment - An object for assigning data to processors.
// .SECTION Description
// A vtkPVAssignment holds a data piece/extent specification for a process.

#ifndef __vtkPVAssignment_h
#define __vtkPVAssignment_h

#include "vtkKWObject.h"
#include "vtkExtentTranslator.h"
#include "vtkPVImage.h"

class vtkPVApplication;


class VTK_EXPORT vtkPVAssignment : public vtkKWObject
{
public:
  static vtkPVAssignment* New();
  vtkTypeMacro(vtkPVAssignment,vtkKWObject);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.  It also sets the default
  // assignment based on processor id.
  void Clone(vtkPVApplication *app);
  
  void SetPiece(int piece, int numPieces);
  int GetPiece() {return this->Piece;}
  int GetNumberOfPieces() {return this->NumberOfPieces;}

  // Description:
  // The original source supplies this input so we will
  // always know the whole extent.  Set broadcasts executes on all processes.
  void SetOriginalImage(vtkPVImage *pvImage);
  vtkPVImage *GetOriginalImage() {return this->OriginalImage;}
  
  // Description:
  // The extent is computetd from then piece and whole extent.
  int *GetExtent();

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
protected:
  vtkPVAssignment();
  ~vtkPVAssignment();
  vtkPVAssignment(const vtkPVAssignment&) {};
  void operator=(const vtkPVAssignment&) {};
  
  int Piece;
  int NumberOfPieces;

  vtkExtentTranslator *Translator;  
  vtkPVImage *OriginalImage;
  int Extent[6];
};

#endif
