/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWTesting.h"

#include "vtkObjectFactory.h"
#include "vtkKWView.h"
#include "vtkImageAppend.h"
#include "vtkImageData.h"
#include "vtkPNGReader.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTesting );
vtkCxxRevisionMacro(vtkKWTesting, "1.1");
vtkCxxSetObjectMacro(vtkKWTesting,RenderView,vtkKWView);

//----------------------------------------------------------------------------
vtkKWTesting::vtkKWTesting()
{
  this->Testing = vtkTesting::New();
  this->Testing->SetBorderOffset(17);
  this->RenderView = 0;
  this->ComparisonImage = 0;
  this->AppendFilter = 0;
}

//----------------------------------------------------------------------------
vtkKWTesting::~vtkKWTesting()
{
  this->SetRenderView(0);
  this->Testing->Delete();
  this->Testing = 0;
  this->SetComparisonImage(0);
  if (this->AppendFilter)
    {
    this->AppendFilter->Delete();
    this->AppendFilter = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWTesting::AddArgument(const char* arg)
{
  this->Testing->AddArgument(arg);
}

//----------------------------------------------------------------------------
int vtkKWTesting::RegressionTest(float thresh)
{
  int res = vtkTesting::FAILED;
  if ( this->RenderView )
    {
    cout << "Compare render window" << endl;
    this->Testing->SetRenderWindow(this->RenderView->GetRenderWindow());
    res = this->Testing->RegressionTest(thresh);
    }
  if ( this->ComparisonImage )
    {
    cout << "Compare image: " << this->ComparisonImage << endl;
    vtkPNGReader* reader = vtkPNGReader::New();
    reader->SetFileName(this->ComparisonImage);
    reader->Update();
    res = this->Testing->RegressionTest(reader->GetOutput(), thresh);
    reader->Delete();
    }
  if ( this->AppendFilter )
    {
    this->AppendFilter->Update();
    res = 
      this->Testing->RegressionTest(this->AppendFilter->GetOutput(), thresh);
    }
  return res != vtkTesting::PASSED;
}


//----------------------------------------------------------------------------
void vtkKWTesting::AppendTestImage(vtkKWView *renderView)
{
  if (!renderView)
    {
    return;
    }
  
  if (!this->AppendFilter)
    {
    this->AppendFilter = vtkImageAppend::New();
    }

  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(renderView->GetRenderWindow());
  w2i->Update();
  
  this->AppendFilter->AddInput(w2i->GetOutput());
  w2i->GetOutput()->SetSource(0);
  w2i->Delete();
}

//----------------------------------------------------------------------------
void vtkKWTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "ComparisonImage: " 
    << (this->ComparisonImage?this->ComparisonImage:"(none)") << endl;
}


