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
// .NAME vtkKWTesting - a frame with a scroll bar
// .SECTION Description
// The ScrollableFrame creates a frame with an attached scrollbar


#ifndef __vtkKWTesting_h
#define __vtkKWTesting_h

#include "vtkObject.h"

class vtkTesting;
class vtkKWView;
class vtkImageAppend;

class VTK_EXPORT vtkKWTesting : public vtkObject
{
public:
  static vtkKWTesting* New();
  vtkTypeRevisionMacro(vtkKWTesting,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set get the render view
  virtual void SetRenderView(vtkKWView* view);
  vtkGetObjectMacro(RenderView, vtkKWView);

  // Description:
  // Set the comparison image file name.
  vtkSetStringMacro(ComparisonImage);

  // Description:
  // Add argument
  virtual void AddArgument(const char* arg);

  // Description:
  // Perform the actual test.
  virtual int RegressionTest(float thresh);

  // Description: Append a test image. Thsi is useful for combining multiple
  // tests into one test with a large image. Thsi method will append test
  // images together (left to right) in order to make a large valid image
  virtual void AppendTestImage(vtkKWView *RenderView);

protected:
  vtkKWTesting();
  ~vtkKWTesting();

  vtkTesting* Testing;
  vtkKWView* RenderView;
  char* ComparisonImage;
  
  vtkImageAppend *AppendFilter;
  
private:
  vtkKWTesting(const vtkKWTesting&); // Not implemented
  void operator=(const vtkKWTesting&); // Not implemented
};


#endif




