/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkMPEG2Writer.h"

#include "vtkImageData.h"
#include "vtkMPEG2WriterHelper.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMPEG2Writer);
vtkCxxRevisionMacro(vtkMPEG2Writer, "1.1.2.1");

//---------------------------------------------------------------------------
vtkMPEG2Writer::vtkMPEG2Writer()
{
  this->MPEG2WriterHelper = vtkMPEG2WriterHelper::New();
}

//---------------------------------------------------------------------------
vtkMPEG2Writer::~vtkMPEG2Writer()
{
  this->MPEG2WriterHelper->Delete();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Start()
{
  this->MPEG2WriterHelper->SetFileName(this->FileName);
  this->MPEG2WriterHelper->Start();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Write()
{
  this->MPEG2WriterHelper->Write();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::End()
{
  this->MPEG2WriterHelper->End();
}

//----------------------------------------------------------------------------
void vtkMPEG2Writer::SetInput(vtkImageData *input)
{
  this->Superclass::SetInput(input);
  this->MPEG2WriterHelper->SetInput(input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkMPEG2Writer::GetInput()
{
  return this->MPEG2WriterHelper->GetInput();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

