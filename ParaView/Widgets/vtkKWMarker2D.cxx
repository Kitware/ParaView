/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMarker2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWMarker2D.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMarker2D );
vtkCxxRevisionMacro(vtkKWMarker2D, "1.1");

//-----------------------------------------------------------------------------
vtkKWMarker2D::vtkKWMarker2D()
{
  float pts[][3] = { {  0, -5, 0 },
                     {  0,  5, 0 },
                     { -5,  0, 0 },
                     {  5,  0, 0 } };
  vtkIdType lins[][2] = { { 0, 1 },
                          { 2, 3 } };
  
  vtkPolyData* pd = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  int cc;
  for ( cc = 0; cc < 4; cc ++ )
    {
    points->InsertPoint(cc, pts[cc]);
    }
  
  lines->InsertNextCell(2, lins[0]);
  lines->InsertNextCell(2, lins[1]);
  pd->SetPoints(points);
  pd->SetLines(lines);
  points->Delete();
  lines->Delete();

  vtkPolyDataMapper2D* mapper = vtkPolyDataMapper2D::New();
  mapper->SetInput(pd);
  pd->Delete();
  this->SetMapper(mapper);
  mapper->Delete();
  this->Renderer = 0;
}

//-----------------------------------------------------------------------------
vtkKWMarker2D::~vtkKWMarker2D()
{
}

//-----------------------------------------------------------------------------
void vtkKWMarker2D::SetColor(float r, float g, float b)
{
}

//-----------------------------------------------------------------------------
void vtkKWMarker2D::SetVisibility(int i)
{
}

//-----------------------------------------------------------------------------
void vtkKWMarker2D::SetPosition(float x, float y)
{
  // Move the widget
  float* pos = this->GetPosition();
  cout << "Cursor position: " << pos[0] << " " << pos[1] << endl;
  pos[0] = x;
  pos[1] = 1-y;
  this->GetPosition();
}

//----------------------------------------------------------------------------
void vtkKWMarker2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
