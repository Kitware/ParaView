/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotReaderMap.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotReaderMap - Maps strings to vtkSpyPlotUniReaders
// .SECTION Description
// Extracted from vtkSpyPlotReader
//-----------------------------------------------------------------------------
//=============================================================================
#ifndef __vtkSpyPlotReaderMap_h
#define __vtkSpyPlotReaderMap_h

#include "vtkSystemIncludes.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/map>

class vtkSpyPlotReader;
class vtkSpyPlotUniReader;

class VTK_EXPORT vtkSpyPlotReaderMap
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSpyPlotUniReader*> MapOfStringToSPCTH;
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  MapOfStringToSPCTH Files;
  vtkstd::string MasterFileName;

  void Initialize(const char *file);
  void Clean(vtkSpyPlotUniReader* save);
  vtkSpyPlotUniReader* GetReader(MapOfStringToSPCTH::iterator& it, 
                                 vtkSpyPlotReader* parent);
  void TellReadersToCheck(vtkSpyPlotReader *parent);
};


#endif
