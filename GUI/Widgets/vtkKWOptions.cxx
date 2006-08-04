/*=========================================================================

  Module:    vtkKWOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWOptions.h"

#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOptions );
vtkCxxRevisionMacro(vtkKWOptions, "1.2");

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetCharacterEncodingAsTclOptionValue(int encoding)
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
const char* vtkKWOptions::GetAnchorAsTkOptionValue(int anchor)
{
  switch (anchor)
    {
    case vtkKWOptions::AnchorNorth:
      return "n";
    case vtkKWOptions::AnchorNorthEast:
      return "ne";
    case vtkKWOptions::AnchorEast:
      return "e";
    case vtkKWOptions::AnchorSouthEast:
      return "se";
    case vtkKWOptions::AnchorSouth:
      return "s";
    case vtkKWOptions::AnchorSouthWest:
      return "sw";
    case vtkKWOptions::AnchorWest:
      return "w";
    case vtkKWOptions::AnchorNorthWest:
      return "nw";
    case vtkKWOptions::AnchorCenter:
      return "center";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetAnchorFromTkOptionValue(const char *anchor)
{
  if (!anchor)
    {
    return vtkKWOptions::AnchorUnknown;
    }
  if (!strcmp(anchor, "n"))
    {
    return vtkKWOptions::AnchorNorth;
    }
  if (!strcmp(anchor, "ne"))
    {
    return vtkKWOptions::AnchorNorthEast;
    }
  if (!strcmp(anchor, "e"))
    {
    return vtkKWOptions::AnchorEast;
    }
  if (!strcmp(anchor, "se"))
    {
    return vtkKWOptions::AnchorSouthEast;
    }
  if (!strcmp(anchor, "s"))
    {
    return vtkKWOptions::AnchorSouth;
    }
  if (!strcmp(anchor, "sw"))
    {
    return vtkKWOptions::AnchorSouthWest;
    }
  if (!strcmp(anchor, "w"))
    {
    return vtkKWOptions::AnchorWest;
    }
  if (!strcmp(anchor, "nw"))
    {
    return vtkKWOptions::AnchorNorthWest;
    }
  if (!strcmp(anchor, "center"))
    {
    return vtkKWOptions::AnchorCenter;
    }
  return vtkKWOptions::AnchorUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetReliefAsTkOptionValue(int relief)
{
  switch (relief)
    {
    case vtkKWOptions::ReliefRaised:
      return "raised";
    case vtkKWOptions::ReliefSunken:
      return "sunken";
    case vtkKWOptions::ReliefFlat:
      return "flat";
    case vtkKWOptions::ReliefRidge:
      return "ridge";
    case vtkKWOptions::ReliefSolid:
      return "solid";
    case vtkKWOptions::ReliefGroove:
      return "groove";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetReliefFromTkOptionValue(const char *relief)
{
  if (!relief)
    {
    return vtkKWOptions::ReliefUnknown;
    }
  if (!strcmp(relief, "raised"))
    {
    return vtkKWOptions::ReliefRaised;
    }
  if (!strcmp(relief, "sunken"))
    {
    return vtkKWOptions::ReliefSunken;
    }
  if (!strcmp(relief, "flat"))
    {
    return vtkKWOptions::ReliefFlat;
    }
  if (!strcmp(relief, "ridge"))
    {
    return vtkKWOptions::ReliefRidge;
    }
  if (!strcmp(relief, "solid"))
    {
    return vtkKWOptions::ReliefSolid;
    }
  if (!strcmp(relief, "groove"))
    {
    return vtkKWOptions::ReliefGroove;
    }
  return vtkKWOptions::ReliefUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetJustificationAsTkOptionValue(int justification)
{
  switch (justification)
    {
    case vtkKWOptions::JustificationLeft:
      return "left";
    case vtkKWOptions::JustificationCenter:
      return "center";
    case vtkKWOptions::JustificationRight:
      return "right";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetJustificationFromTkOptionValue(const char *justification)
{
  if (!justification)
    {
    return vtkKWOptions::JustificationUnknown;
    }
  if (!strcmp(justification, "left"))
    {
    return vtkKWOptions::JustificationLeft;
    }
  if (!strcmp(justification, "center"))
    {
    return vtkKWOptions::JustificationCenter;
    }
  if (!strcmp(justification, "right"))
    {
    return vtkKWOptions::JustificationRight;
    }
  return vtkKWOptions::JustificationUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetSelectionModeAsTkOptionValue(int mode)
{
  switch (mode)
    {
    case vtkKWOptions::SelectionModeSingle:
      return "single";
    case vtkKWOptions::SelectionModeBrowse:
      return "browse";
    case vtkKWOptions::SelectionModeMultiple:
      return "multiple";
    case vtkKWOptions::SelectionModeExtended:
      return "extended";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetSelectionModeFromTkOptionValue(const char *mode)
{
  if (!mode)
    {
    return vtkKWOptions::SelectionModeUnknown;
    }
  if (!strcmp(mode, "single"))
    {
    return vtkKWOptions::SelectionModeSingle;
    }
  if (!strcmp(mode, "browse"))
    {
    return vtkKWOptions::SelectionModeBrowse;
    }
  if (!strcmp(mode, "multiple"))
    {
    return vtkKWOptions::SelectionModeMultiple;
    }
  if (!strcmp(mode, "extended"))
    {
    return vtkKWOptions::SelectionModeExtended;
    }
  return vtkKWOptions::SelectionModeUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetOrientationAsTkOptionValue(int orientation)
{
  switch (orientation)
    {
    case vtkKWOptions::OrientationHorizontal:
      return "horizontal";
    case vtkKWOptions::OrientationVertical:
      return "vertical";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetOrientationFromTkOptionValue(const char *orientation)
{
  if (!orientation)
    {
    return vtkKWOptions::OrientationUnknown;
    }
  if (!strcmp(orientation, "horizontal"))
    {
    return vtkKWOptions::OrientationHorizontal;
    }
  if (!strcmp(orientation, "vertical"))
    {
    return vtkKWOptions::OrientationVertical;
    }
  return vtkKWOptions::OrientationUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetStateAsTkOptionValue(int state)
{
  switch (state)
    {
    case vtkKWOptions::StateDisabled:
      return "disabled";
    case vtkKWOptions::StateNormal:
      return "normal";
    case vtkKWOptions::StateActive:
      return "active";
    case vtkKWOptions::StateReadOnly:
      return "readonly";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetStateFromTkOptionValue(const char *state)
{
  if (!state)
    {
    return vtkKWOptions::StateUnknown;
    }
  if (!strcmp(state, "disabled"))
    {
    return vtkKWOptions::StateDisabled;
    }
  if (!strcmp(state, "normal"))
    {
    return vtkKWOptions::StateNormal;
    }
  if (!strcmp(state, "active"))
    {
    return vtkKWOptions::StateActive;
    }
  if (!strcmp(state, "readonly"))
    {
    return vtkKWOptions::StateReadOnly;
    }
  return vtkKWOptions::StateUnknown;
}

//----------------------------------------------------------------------------
const char* vtkKWOptions::GetCompoundModeAsTkOptionValue(int compound)
{
  switch (compound)
    {
    case vtkKWOptions::CompoundModeNone:
      return "none";
    case vtkKWOptions::CompoundModeLeft:
      return "left";
    case vtkKWOptions::CompoundModeCenter:
      return "center";
    case vtkKWOptions::CompoundModeRight:
      return "right";
    case vtkKWOptions::CompoundModeTop:
      return "top";
    case vtkKWOptions::CompoundModeBottom:
      return "bottom";
    default:
      return "";
    }
}

//----------------------------------------------------------------------------
int vtkKWOptions::GetCompoundModeFromTkOptionValue(const char *compound)
{
  if (!compound)
    {
    return vtkKWOptions::CompoundModeUnknown;
    }
  if (!strcmp(compound, "none"))
    {
    return vtkKWOptions::CompoundModeNone;
    }
  if (!strcmp(compound, "left"))
    {
    return vtkKWOptions::CompoundModeLeft;
    }
  if (!strcmp(compound, "center"))
    {
    return vtkKWOptions::CompoundModeCenter;
    }
  if (!strcmp(compound, "right"))
    {
    return vtkKWOptions::CompoundModeRight;
    }
  if (!strcmp(compound, "top"))
    {
    return vtkKWOptions::CompoundModeTop;
    }
  if (!strcmp(compound, "bottom"))
    {
    return vtkKWOptions::CompoundModeBottom;
    }
  return vtkKWOptions::CompoundModeUnknown;
}

//----------------------------------------------------------------------------
void vtkKWOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

