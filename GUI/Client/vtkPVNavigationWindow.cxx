/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNavigationWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVNavigationWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <kwsys/SystemTools.hxx>

#include <stdarg.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVNavigationWindow );
vtkCxxRevisionMacro(vtkPVNavigationWindow, "1.25");

//-----------------------------------------------------------------------------
vtkPVNavigationWindow::vtkPVNavigationWindow()
{
}

//-----------------------------------------------------------------------------
vtkPVNavigationWindow::~vtkPVNavigationWindow()
{
}


//-----------------------------------------------------------------------------
void vtkPVNavigationWindow::ChildUpdate(vtkPVSource *currentSource)
{
  if (currentSource == 0)
    {
    return;
    }
  vtkPVSource *source;
  vtkPVSource **inputs = currentSource->GetPVInputs();
  int numInputs, xMid, yMid=0, y, i;
  int bboxIn[4], bboxOut[4], bboxSource[4];
  static const char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";  

  // Draw the name of the assembly.
  char *text = this->GetTextRepresentation(currentSource);
  const char *res = this->CreateCanvasItem(
    "%s create text %d %d -text {%s} -font %s -tags x",
    this->Canvas->GetWidgetName(), 170, 10, text,font);
  delete [] text;
  char* tmp = kwsys::SystemTools::DuplicateString(res);
  
  if (this->CreateSelectionBindings)
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
      if ( (source = inputs[i]) )
        {
        // Draw the name of the assembly.
        
        char *itext = this->GetTextRepresentation(source);
        const char* cres = this->CreateCanvasItem(
          "%s create text %d %d -text {%s} -font %s -anchor e "
          "-tags x -fill blue",
          this->Canvas->GetWidgetName(), bboxSource[0]-50, y, itext, font);
        delete [] itext;
        tmp = kwsys::SystemTools::DuplicateString(cres);
        
        this->CalculateBBox(this->Canvas, tmp, bboxIn);
        if (this->CreateSelectionBindings)
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
          if (source->GetPVInput(0))
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

  y = 10;
  // only want to set xMid  once
  xMid = bboxSource[2] + 25;
  int num;
  num = currentSource->GetNumberOfPVConsumers();
  for (i = 0; i < num; i++)
    {
    source = currentSource->GetPVConsumer(i);
    
    // Draw the name of the assembly .
    char *otext = this->GetTextRepresentation(source);
    const char* nres = this->CreateCanvasItem(
      "%s create text %d %d -text {%s} -font %s -anchor w "
      "-tags x -fill blue",
      this->Canvas->GetWidgetName(), bboxSource[2]+50, y, otext, font);
    delete [] otext;
    tmp = kwsys::SystemTools::DuplicateString(nres);
    
    // Get the bounding box for the name.
    this->CalculateBBox(this->Canvas, tmp, bboxOut);
    if (this->CreateSelectionBindings)
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
    if (source->GetNumberOfPVConsumers() > 0)
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
      
    y += 15;
    }

}

//-----------------------------------------------------------------------------
void vtkPVNavigationWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
