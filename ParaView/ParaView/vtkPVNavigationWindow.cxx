/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVNavigationWindow.cxx
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
#include "vtkPVNavigationWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkString.h"

#include <stdarg.h>

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVNavigationWindow );

vtkPVNavigationWindow::vtkPVNavigationWindow()
{
}

vtkPVNavigationWindow::~vtkPVNavigationWindow()
{
}


void vtkPVNavigationWindow::ChildUpdate(vtkPVSource *currentSource, int NoBind)
{
  vtkPVSource *source;
  vtkPVData **inputs = currentSource->GetPVInputs();
  vtkPVData **outputs;
  int numInputs, xMid, yMid=0, y, i;
  int bboxIn[4], bboxOut[4], bboxSource[4];
  vtkPVData *moreOut;
  static const char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";  

  // Draw the name of the assembly.
  const char *res = this->CreateCanvasItem(
    "%s create text %d %d -text {%s} -font %s -tags x",
    this->Canvas->GetWidgetName(), 170, 10, currentSource->GetName(),font);
  char* tmp = vtkString::Duplicate(res);
  
  if ( !NoBind )
    {
    this->Script("%s bind %s <ButtonPress-3> "
                 "{ %s DisplayModulePopupMenu %s %%X %%Y }",
                 this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
                 currentSource->GetTclName());
    }
  // Get the bounding box for the name.
  this->CalculateBBox(this->Canvas, tmp, bboxSource);
  delete [] tmp;
  tmp = 0;

  // Put the inputs in the canvas.
  if (inputs)
    {
    y = 10;
    numInputs = currentSource->GetNumberOfPVInputs();
    // only want to set xMid once
    xMid = bboxSource[0] - 25;
    for (i = 0; i < numInputs; i++)
      {
      if (inputs[i] && (source = inputs[i]->GetPVSource()) )
        {
        // Draw the name of the assembly.
        
        const char* res = this->CreateCanvasItem(
          "%s create text %d %d -text {%s} -font %s -anchor e "
          "-tags x -fill blue",
          this->Canvas->GetWidgetName(), bboxSource[0]-50, y,
          source->GetName(), font);
        tmp = vtkString::Duplicate(res);
        
        this->CalculateBBox(this->Canvas, tmp, bboxIn);
        if ( !NoBind )
          {
          this->Script("%s bind %s <ButtonPress-1> {%s SetCurrentPVSourceCallback %s}",
                       this->Canvas->GetWidgetName(), tmp,
                       currentSource->GetPVWindow()->GetTclName(), 
                       source->GetTclName());
          this->Script("%s bind %s <Enter> {%s HighlightObject %s 1}",
                       this->Canvas->GetWidgetName(), tmp,
                       this->GetTclName(), tmp);
          this->Script("%s bind %s <Leave> {%s HighlightObject %s 0}",
                       this->Canvas->GetWidgetName(), tmp,
                     this->GetTclName(), tmp);
          this->Script("%s bind %s <ButtonPress-3> "
                       "{ %s DisplayModulePopupMenu %s %%X %%Y }",
                       this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
                       source->GetTclName());
          }
        
        delete [] tmp;
        tmp = 0;

        // only want to set yMid once
        if ( i == 0 )
          {
          yMid = static_cast<int>(0.5 * (bboxIn[1]+bboxIn[3]));
          }

        // Draw a line from input to source.
        if (y == 10)
          {
          this->CreateCanvasItem(
            "%s create line %d %d %d %d -fill gray50 -arrow last",
            this->Canvas->GetWidgetName(), bboxIn[2], yMid,
            bboxSource[0], yMid);
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), bboxIn[2],
                       yMid, xMid, yMid);
          }
        
        if (source->GetPVInputs())
          {
          if (source->GetPVInput()->GetPVSource())
            {
            // Draw ellipsis indicating that this source has a source.
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(), bboxIn[0] - 18, yMid, 
                         bboxIn[0] - 16, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(), bboxIn[0] - 14, yMid, 
                         bboxIn[0] - 12, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(), bboxIn[0] - 10, yMid, 
                         bboxIn[0] - 8, yMid);
            }
          }
        }
      y += 15;
      }
    }

  // Put the outputs in the canvas.
  outputs = currentSource->GetPVOutputs();
  int numOutputs = currentSource->GetNumberOfPVOutputs();
  if (outputs)
    {
    y = 10;
    if (outputs[0])
      {
      // only want to set xMid  once
      xMid = bboxSource[2] + 25;
      // We have two display modes:
      // 1. If the number of outputs is > 1, we display the first
      // consumer of each output (with the current implementation,
      // there is no way of assigning more than one consumer to
      // an output from the user interface if there are more than
      // 1 outputs)
      // 2. If there is only one output, we display all the consumers
      // of that output
      int num;
      if (numOutputs > 1)
        {
        num = numOutputs;
        }
      else
        {
        num = outputs[0]->GetNumberOfPVConsumers();
        }
      for (i = 0; i < num; i++)
        {
        if (numOutputs > 1)
          {
          if ( outputs[i]->GetNumberOfPVConsumers() == 0 )
            {
            continue;
            }
          source = outputs[i]->GetPVConsumer(0);
          }
        else
          {
          source = outputs[0]->GetPVConsumer(i);
          }
        
        // Draw the name of the assembly .
        const char* res = this->CreateCanvasItem(
          "%s create text %d %d -text {%s} -font %s -anchor w "
          "-tags x -fill blue",
          this->Canvas->GetWidgetName(), bboxSource[2]+50, y,
          source->GetName(), font);
        tmp = vtkString::Duplicate(res);
        
        // Get the bounding box for the name.
        this->CalculateBBox(this->Canvas, tmp, bboxOut);
        if ( !NoBind )
          {
          this->Script("%s bind %s <ButtonPress-1> {%s  SetCurrentPVSourceCallback %s}",
                       this->Canvas->GetWidgetName(), tmp,
                       currentSource->GetPVWindow()->GetTclName(), 
                       source->GetTclName());
          this->Script("%s bind %s <Enter> {%s HighlightObject %s 1}",
                       this->Canvas->GetWidgetName(), tmp,
                       this->GetTclName(), tmp);
          this->Script("%s bind %s <Leave> {%s HighlightObject %s 0}",
                       this->Canvas->GetWidgetName(), tmp,
                       this->GetTclName(), tmp); 
          this->Script("%s bind %s <ButtonPress-3> "
                       "{ %s DisplayModulePopupMenu %s %%X %%Y }",
                       this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
                       source->GetTclName());
          }
        delete [] tmp;
        tmp = NULL;
        
        // only want to set yMid once
        if ( i == 0 )
          {
          yMid = static_cast<int>(0.5 * (bboxOut[1]+bboxOut[3]));
          }

        // Draw to output.
        if (y == 10)
          { // first is a special case (single line).
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->Canvas->GetWidgetName(), bboxSource[2],
                       yMid, bboxOut[0], yMid);
          }
        else
          {
          xMid = (int)(0.5 * (bboxSource[2] + bboxOut[0]));
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       bboxOut[0], yMid);
          }
        if ((moreOut = source->GetPVOutput(0)))
          {
          if (moreOut->GetNumberOfPVConsumers() > 0)
            {
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+10, yMid, bboxOut[2]+12, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+14, yMid, bboxOut[2]+16, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+18, yMid, bboxOut[2]+20, yMid);
            }
          }
        y += 15;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVNavigationWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
