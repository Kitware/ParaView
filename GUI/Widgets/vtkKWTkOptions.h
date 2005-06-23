/*=========================================================================

  Module:    vtkKWTkOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTkOptions - set of methods to convert to/from Tk options
// .SECTION Description
// This class provides some conversion betweek vtkKWWidget constants
// and the corresponding Tk options.

#ifndef __vtkKWTkOptions_h
#define __vtkKWTkOptions_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class KWWIDGETS_EXPORT vtkKWTkOptions : public vtkObject
{
public:
  static vtkKWTkOptions* New();
  vtkTypeRevisionMacro(vtkKWTkOptions,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the Tcl value for a given encoding constant
  // Check vtkSystemIncludes for a list of valid encodings.
  static const char* GetCharacterEncodingAsTclOptionValue(int);

  // Description:
  // Return the Tk value for a given anchor constant, and vice-versa
  // Specifies how the information in a widget (e.g. text or a bitmap) is to
  // be displayed in the widget.
  //BTX
  enum AnchorType
  {
    AnchorNorth = 0,
    AnchorNorthEast,
    AnchorEast,
    AnchorSouthEast,
    AnchorSouth,
    AnchorSouthWest,
    AnchorWest,
    AnchorNorthWest,
    AnchorCenter,
    AnchorUnknown
  };
  //ETX
  static const char* GetAnchorAsTkOptionValue(int);
  static int GetAnchorFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given relief constant, and vice-versa
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  //BTX
  enum ReliefType
  {
    ReliefRaised = 0,
    ReliefSunken,
    ReliefFlat,
    ReliefRidge,
    ReliefSolid,
    ReliefGroove,
    ReliefUnknown
  };
  //ETX
  static const char* GetReliefAsTkOptionValue(int);
  static int GetReliefFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given justification constant, and vice-versa.
  // When there are multiple lines of text displayed in a widget, this option
  // determines how the lines line up with each other.   
  //BTX
  enum JustificationType
  {
    JustificationLeft = 0,
    JustificationCenter,
    JustificationRight,
    JustificationUnknown
  };
  //ETX
  static const char* GetJustificationAsTkOptionValue(int);
  static int GetJustificationFromTkOptionValue(const char *);

  // Description:
  // Set/Get the one of several styles for manipulating the selection. 
  //BTX
  enum SelectionModeType
  {
    SelectionModeSingle = 0,
    SelectionModeBrowse,
    SelectionModeMultiple,
    SelectionModeExtended,
    SelectionModeUnknown
  };
  //ETX
  static const char* GetSelectionModeAsTkOptionValue(int);
  static int GetSelectionModeFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given orientation constant, and vice-versa.
  // For widgets that can lay themselves out with either a horizontal or
  // vertical orientation, such as scrollbars, this option specifies which 
  // orientation should be used. 
  //BTX
  enum OrientationType
  {
    OrientationHorizontal = 0,
    OrientationVertical,
    OrientationUnknown
  };
  //ETX
  static const char* GetOrientationAsTkOptionValue(int);
  static int GetOrientationFromTkOptionValue(const char *);

protected:
  vtkKWTkOptions() {};
  ~vtkKWTkOptions() {};

private:
  
  vtkKWTkOptions(const vtkKWTkOptions&); // Not implemented
  void operator=(const vtkKWTkOptions&); // Not implemented
};

#endif
