/*=========================================================================

  Module:    vtkKWTkOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWTkOptions.h"

#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTkOptions );
vtkCxxRevisionMacro(vtkKWTkOptions, "1.3");

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetCharacterEncodingAsTclOptionValue(int encoding)
{
  switch (encoding)
    {
    case VTK_ENCODING_US_ASCII:
      return "ascii";

    case VTK_ENCODING_UNICODE:
      return "unicode";

    case VTK_ENCODING_UTF_8:
      return "utf-8";

    case VTK_ENCODING_ISO_8859_1:
      return "iso8859-1";

    case VTK_ENCODING_ISO_8859_2:
      return "iso8859-2";

    case VTK_ENCODING_ISO_8859_3:
      return "iso8859-3";

    case VTK_ENCODING_ISO_8859_4:
      return "iso8859-4";

    case VTK_ENCODING_ISO_8859_5:
      return "iso8859-5";

    case VTK_ENCODING_ISO_8859_6:
      return "iso8859-5";

    case VTK_ENCODING_ISO_8859_7:
      return "iso8859-7";

    case VTK_ENCODING_ISO_8859_8:
      return "iso8859-8";

    case VTK_ENCODING_ISO_8859_9:
      return "iso8859-9";

    case VTK_ENCODING_ISO_8859_10:
      return "iso8859-10";

    case VTK_ENCODING_ISO_8859_11:
      return "iso8859-11";

    case VTK_ENCODING_ISO_8859_12:
      return "iso8859-12";

    case VTK_ENCODING_ISO_8859_13:
      return "iso8859-13";

    case VTK_ENCODING_ISO_8859_14:
      return "iso8859-14";

    case VTK_ENCODING_ISO_8859_15:
      return "iso8859-15";

    case VTK_ENCODING_ISO_8859_16:
      return "iso8859-16";

    default:
      return "identity";
    }
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetAnchorAsTkOptionValue(int anchor)
{
  switch (anchor)
    {
    case vtkKWTkOptions::AnchorNorth:
      return "n";
    case vtkKWTkOptions::AnchorNorthEast:
      return "ne";
    case vtkKWTkOptions::AnchorEast:
      return "e";
    case vtkKWTkOptions::AnchorSouthEast:
      return "se";
    case vtkKWTkOptions::AnchorSouth:
      return "s";
    case vtkKWTkOptions::AnchorSouthWest:
      return "sw";
    case vtkKWTkOptions::AnchorWest:
      return "w";
    case vtkKWTkOptions::AnchorNorthWest:
      return "nw";
    case vtkKWTkOptions::AnchorCenter:
      return "center";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetAnchorFromTkOptionValue(const char *anchor)
{
  if (!anchor)
    {
    return vtkKWTkOptions::AnchorUnknown;
    }
  if (!strcmp(anchor, "n"))
    {
    return vtkKWTkOptions::AnchorNorth;
    }
  if (!strcmp(anchor, "ne"))
    {
    return vtkKWTkOptions::AnchorNorthEast;
    }
  if (!strcmp(anchor, "e"))
    {
    return vtkKWTkOptions::AnchorEast;
    }
  if (!strcmp(anchor, "se"))
    {
    return vtkKWTkOptions::AnchorSouthEast;
    }
  if (!strcmp(anchor, "s"))
    {
    return vtkKWTkOptions::AnchorSouth;
    }
  if (!strcmp(anchor, "sw"))
    {
    return vtkKWTkOptions::AnchorSouthWest;
    }
  if (!strcmp(anchor, "w"))
    {
    return vtkKWTkOptions::AnchorWest;
    }
  if (!strcmp(anchor, "nw"))
    {
    return vtkKWTkOptions::AnchorNorthWest;
    }
  if (!strcmp(anchor, "center"))
    {
    return vtkKWTkOptions::AnchorCenter;
    }
  return vtkKWTkOptions::AnchorUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetReliefAsTkOptionValue(int relief)
{
  switch (relief)
    {
    case vtkKWTkOptions::ReliefRaised:
      return "raised";
    case vtkKWTkOptions::ReliefSunken:
      return "sunken";
    case vtkKWTkOptions::ReliefFlat:
      return "flat";
    case vtkKWTkOptions::ReliefRidge:
      return "ridge";
    case vtkKWTkOptions::ReliefSolid:
      return "solid";
    case vtkKWTkOptions::ReliefGroove:
      return "groove";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetReliefFromTkOptionValue(const char *relief)
{
  if (!relief)
    {
    return vtkKWTkOptions::ReliefUnknown;
    }
  if (!strcmp(relief, "raised"))
    {
    return vtkKWTkOptions::ReliefRaised;
    }
  if (!strcmp(relief, "sunken"))
    {
    return vtkKWTkOptions::ReliefSunken;
    }
  if (!strcmp(relief, "flat"))
    {
    return vtkKWTkOptions::ReliefFlat;
    }
  if (!strcmp(relief, "ridge"))
    {
    return vtkKWTkOptions::ReliefRidge;
    }
  if (!strcmp(relief, "solid"))
    {
    return vtkKWTkOptions::ReliefSolid;
    }
  if (!strcmp(relief, "groove"))
    {
    return vtkKWTkOptions::ReliefGroove;
    }
  return vtkKWTkOptions::ReliefUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetJustificationAsTkOptionValue(int justification)
{
  switch (justification)
    {
    case vtkKWTkOptions::JustificationLeft:
      return "left";
    case vtkKWTkOptions::JustificationCenter:
      return "center";
    case vtkKWTkOptions::JustificationRight:
      return "right";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetJustificationFromTkOptionValue(const char *justification)
{
  if (!justification)
    {
    return vtkKWTkOptions::JustificationUnknown;
    }
  if (!strcmp(justification, "left"))
    {
    return vtkKWTkOptions::JustificationLeft;
    }
  if (!strcmp(justification, "center"))
    {
    return vtkKWTkOptions::JustificationCenter;
    }
  if (!strcmp(justification, "right"))
    {
    return vtkKWTkOptions::JustificationRight;
    }
  return vtkKWTkOptions::JustificationUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetSelectionModeAsTkOptionValue(int mode)
{
  switch (mode)
    {
    case vtkKWTkOptions::SelectionModeSingle:
      return "single";
    case vtkKWTkOptions::SelectionModeBrowse:
      return "browse";
    case vtkKWTkOptions::SelectionModeMultiple:
      return "multiple";
    case vtkKWTkOptions::SelectionModeExtended:
      return "extended";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetSelectionModeFromTkOptionValue(const char *mode)
{
  if (!mode)
    {
    return vtkKWTkOptions::SelectionModeUnknown;
    }
  if (!strcmp(mode, "single"))
    {
    return vtkKWTkOptions::SelectionModeSingle;
    }
  if (!strcmp(mode, "browse"))
    {
    return vtkKWTkOptions::SelectionModeBrowse;
    }
  if (!strcmp(mode, "multiple"))
    {
    return vtkKWTkOptions::SelectionModeMultiple;
    }
  if (!strcmp(mode, "extended"))
    {
    return vtkKWTkOptions::SelectionModeExtended;
    }
  return vtkKWTkOptions::SelectionModeUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetOrientationAsTkOptionValue(int orientation)
{
  switch (orientation)
    {
    case vtkKWTkOptions::OrientationHorizontal:
      return "horizontal";
    case vtkKWTkOptions::OrientationVertical:
      return "vertical";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetOrientationFromTkOptionValue(const char *orientation)
{
  if (!orientation)
    {
    return vtkKWTkOptions::OrientationUnknown;
    }
  if (!strcmp(orientation, "horizontal"))
    {
    return vtkKWTkOptions::OrientationHorizontal;
    }
  if (!strcmp(orientation, "vertical"))
    {
    return vtkKWTkOptions::OrientationVertical;
    }
  return vtkKWTkOptions::OrientationUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWTkOptions::GetStateAsTkOptionValue(int state)
{
  switch (state)
    {
    case vtkKWTkOptions::StateDisabled:
      return "disabled";
    case vtkKWTkOptions::StateNormal:
      return "normal";
    case vtkKWTkOptions::StateActive:
      return "active";
    case vtkKWTkOptions::StateReadOnly:
      return "readonly";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWTkOptions::GetStateFromTkOptionValue(const char *state)
{
  if (!state)
    {
    return vtkKWTkOptions::StateUnknown;
    }
  if (!strcmp(state, "disabled"))
    {
    return vtkKWTkOptions::StateDisabled;
    }
  if (!strcmp(state, "normal"))
    {
    return vtkKWTkOptions::StateNormal;
    }
  if (!strcmp(state, "active"))
    {
    return vtkKWTkOptions::StateActive;
    }
  if (!strcmp(state, "readonly"))
    {
    return vtkKWTkOptions::StateReadOnly;
    }
  return vtkKWTkOptions::StateUnknown;
}

//----------------------------------------------------------------------------
void vtkKWTkOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

