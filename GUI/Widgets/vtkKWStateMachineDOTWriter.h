/*=========================================================================

  Module:    vtkKWStateMachineDOTWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineDOTWriter - a state machine DOT writer.
// .SECTION Description
// This class is a state machine writer for the DOT format, based on the
// reference document: http://www.graphviz.org/Documentation/dotguide.pdf
// A state machine is defined by a set of states, a set of inputs and a
// transition matrix that defines for each pair of (state,input) what is
// the next state to assume.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWStateMachineWriter vtkKWStateMachine

#ifndef __vtkKWStateMachineDOTWriter_h
#define __vtkKWStateMachineDOTWriter_h

#include "vtkKWStateMachineWriter.h"

class KWWidgets_EXPORT vtkKWStateMachineDOTWriter : public vtkKWStateMachineWriter
{
public:
  static vtkKWStateMachineDOTWriter* New();
  vtkTypeRevisionMacro(vtkKWStateMachineDOTWriter, vtkKWStateMachineWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Write state machine representation to stream. 
  // Returns 1 on success and 0 on failure.
  virtual int WriteToStream(ostream& os);

  // Description:
  // Set/Get the font name used for state labels. Defaults to Helvetica.
  // It is best to stick to Times, Helvetica, Courier or Symbol
  vtkGetStringMacro(StateFontName);
  vtkSetStringMacro(StateFontName);

  // Description:
  // Set/Get the font size used for state labels. Defaults to 12.
  vtkSetClampMacro(StateFontSize, int, 2, 200);
  vtkGetMacro(StateFontSize, int);

  // Description:
  // Set/Get the font color used for state labels. Defaults to black.
  vtkSetVector3Macro(StateFontColor,double);
  vtkGetVector3Macro(StateFontColor,double);

  // Description:
  // Set/Get the font name used for input labels. Defaults to Helvetica.
  // It is best to stick to Times, Helvetica, Courier or Symbol
  vtkGetStringMacro(InputFontName);
  vtkSetStringMacro(InputFontName);

  // Description:
  // Set/Get the font size used for input labels. Defaults to 9.
  vtkSetClampMacro(InputFontSize, int, 2, 200);
  vtkGetMacro(InputFontSize, int);

  // Description:
  // Set/Get the font color used for input labels. Defaults to blue.
  vtkSetVector3Macro(InputFontColor,double);
  vtkGetVector3Macro(InputFontColor,double);

  // Description:
  // Set/Get graph label (at the bottom of the graph).
  vtkGetStringMacro(GraphLabel);
  vtkSetStringMacro(GraphLabel);

  // Description:
  // Set/Get the font name used for graph labels. Defaults to Helvetica.
  // Graph labels do not include state or input labels.
  // It is best to stick to Times, Helvetica, Courier or Symbol
  vtkGetStringMacro(GraphFontName);
  vtkSetStringMacro(GraphFontName);

  // Description:
  // Set/Get the font size used for graph labels. Defaults to 14.
  // Graph labels do not include state or input labels.
  vtkSetClampMacro(GraphFontSize, int, 2, 200);
  vtkGetMacro(GraphFontSize, int);

  // Description:
  // Set/Get the font color used for graph labels. Defaults to black.
  // Graph labels do not include state or input labels.
  vtkSetVector3Macro(GraphFontColor,double);
  vtkGetVector3Macro(GraphFontColor,double);

  // Description:
  // Set/Get the preferred graph direction. Defaults to left to right.
  //BTX
  enum
  {
    GraphDirectionTopToBottom,
    GraphDirectionLeftToRight
  };
  //ETX
  vtkSetClampMacro(GraphDirection, int, GraphDirectionTopToBottom, GraphDirectionLeftToRight);
  vtkGetMacro(GraphDirection, int);
  virtual void SetGraphDirectionToTopToBottom();
  virtual void SetGraphDirectionToLeftToRight();

  // Description:
  // Set/Get the font name used for cluster labels. Defaults to Helvetica.
  // It is best to stick to Times, Helvetica, Courier or Symbol
  vtkGetStringMacro(ClusterFontName);
  vtkSetStringMacro(ClusterFontName);

  // Description:
  // Set/Get the font size used for cluster labels. Defaults to 10.
  vtkSetClampMacro(ClusterFontSize, int, 2, 200);
  vtkGetMacro(ClusterFontSize, int);

  // Description:
  // Set/Get the font color used for cluster labels. Defaults to black.
  vtkSetVector3Macro(ClusterFontColor,double);
  vtkGetVector3Macro(ClusterFontColor,double);

protected:
  vtkKWStateMachineDOTWriter();
  ~vtkKWStateMachineDOTWriter();

  char *GraphLabel;
  char *GraphFontName;
  int GraphFontSize;
  double GraphFontColor[3];
  int GraphDirection;

  char *StateFontName;
  int StateFontSize;
  double StateFontColor[3];

  char *InputFontName;
  int InputFontSize;
  double InputFontColor[3];

  char *ClusterFontName;
  int ClusterFontSize;
  double ClusterFontColor[3];

private:

  vtkKWStateMachineDOTWriter(const vtkKWStateMachineDOTWriter&); // Not implemented
  void operator=(const vtkKWStateMachineDOTWriter&); // Not implemented
};

#endif
