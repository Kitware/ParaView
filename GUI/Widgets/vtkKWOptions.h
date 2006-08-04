/*=========================================================================

  Module:    vtkKWOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWOptions - set of common options.
// .SECTION Description
// This class also provides some conversion betweek vtkKWWidget constants
// and the corresponding Tk options.

#ifndef __vtkKWOptions_h
#define __vtkKWOptions_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class KWWidgets_EXPORT vtkKWOptions : public vtkObject
{
public:
  static vtkKWOptions* New();
  vtkTypeRevisionMacro(vtkKWOptions,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
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

  // Description:
  // Specifies the 3-D effect desired for the widget. The value indicates how
  // the interior of the widget should appear relative to its exterior. 
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

  // Description:
  // When there are multiple lines of text displayed in a widget, 
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

  // Description:
  // Specifies one of several styles for manipulating the selection.
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

  // Description:
  // For widgets that can lay themselves out with either a horizontal or
  // vertical orientation, such as scales or scrollbars, specifies which 
  // orientation should be used. 
  //BTX
  enum OrientationType
  {
    OrientationHorizontal = 0,
    OrientationVertical,
    OrientationUnknown
  };
  //ETX

  // Description:
  // Specifies the state of a widget.
  //BTX
  enum StateType
  {
    StateDisabled = 0,
    StateNormal = 1,
    StateActive = 2,
    StateReadOnly = 3,
    StateUnknown
  };
  //ETX

  // Description:
  // Specifies if the widget should display text and bitmaps/images at the
  // same time, and if so, where the bitmap/image should be placed relative 
  // to the text. 
  //BTX
  enum CompoundModeType
  {
    CompoundModeNone = 0,
    CompoundModeLeft,
    CompoundModeCenter,
    CompoundModeRight,
    CompoundModeTop,
    CompoundModeBottom,
    CompoundModeUnknown
  };
  //ETX

  // Description:
  // Return the Tcl value for a given encoding constant
  // Check vtkSystemIncludes for a list of valid encodings.
  static const char* GetCharacterEncodingAsTclOptionValue(int);

  // Description:
  // Return the Tk value for a given anchor constant, and vice-versa
  static const char* GetAnchorAsTkOptionValue(int);
  static int GetAnchorFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given relief constant, and vice-versa
  static const char* GetReliefAsTkOptionValue(int);
  static int GetReliefFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given justification constant, and vice-versa.
  static const char* GetJustificationAsTkOptionValue(int);
  static int GetJustificationFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given selection mode constant, and vice-versa.
  static const char* GetSelectionModeAsTkOptionValue(int);
  static int GetSelectionModeFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given orientation constant, and vice-versa.
  static const char* GetOrientationAsTkOptionValue(int);
  static int GetOrientationFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given state constant, and vice-versa.
  static const char* GetStateAsTkOptionValue(int);
  static int GetStateFromTkOptionValue(const char *);

  // Description:
  // Return the Tk value for a given compound constant, and vice-versa.
  static const char* GetCompoundModeAsTkOptionValue(int);
  static int GetCompoundModeFromTkOptionValue(const char *);

protected:
  vtkKWOptions() {};
  ~vtkKWOptions() {};

private:
  
  vtkKWOptions(const vtkKWOptions&); // Not implemented
  void operator=(const vtkKWOptions&); // Not implemented
};

#endif
