/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkPVPolyDataMapper.h"


//----------------------------------------------------------------------------
vtkPVPolyDataMapper::vtkPVPolyDataMapper()
{
  this->Assignment = NULL;
}

//----------------------------------------------------------------------------
vtkPVPolyDataMapper::~vtkPVPolyDataMapper()
{
  this->SetAssignment(NULL);
}

//----------------------------------------------------------------------------
vtkPVPolyDataMapper* vtkPVPolyDataMapper::New()
{
  return new vtkPVPolyDataMapper();
}

//----------------------------------------------------------------------------
void vtkPVPolyDataMapper::Update()
{
  vtkPolyData *pd;
  
  pd = this->GetInput();
  if (this->Assignment && pd)
    {
    pd->SetUpdateNumberOfPieces(this->Assignment->GetNumberOfPieces());
    pd->SetUpdatePiece(this->Assignment->GetPiece());
    }
  
  this->vtkMesaPolyDataMapper::Update();
}

//----------------------------------------------------------------------------
float *vtkPVPolyDataMapper::GetBounds()
{
  vtkPolyData *pd;
  
  pd = this->GetInput();
  if (this->Assignment && pd)
    {
    pd->SetUpdateNumberOfPieces(this->Assignment->GetNumberOfPieces());
    pd->SetUpdatePiece(this->Assignment->GetPiece());
    }
  
  return this->vtkMesaPolyDataMapper::GetBounds();
}



