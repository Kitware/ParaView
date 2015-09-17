/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FieldDisplacementMapData_h
#define FieldDisplacementMapData_h

#include "FieldTraceData.h"

class vtkDataSet;
class vtkFloatArray;

/// Records the end point(displacement) of a stream line.
/**
The displacement map, maps the end point(s) of a streamline onto
the seed geometry. This map is used in FTLE computations.
*/
class FieldDisplacementMapData : public FieldTraceData
{
public:
  FieldDisplacementMapData();
  virtual ~FieldDisplacementMapData();

  /**
  Copy the IntersectColor and SourceId array into the output.
  */
  virtual void SetOutput(vtkDataSet *o);

  /**
  Move scalar data (IntersectColor, SourceId) from the internal
  structure into the vtk output data.
  */
  virtual int SyncScalars();

  /**
  Move field line geometry (trace) from the internal structure
  into the vtk output data.
  */
  virtual int SyncGeometry(){ return 0; }

protected:
  vtkFloatArray *Displacement;
  vtkFloatArray *FwdDisplacement;
  vtkFloatArray *BwdDisplacement;
};

#endif

// VTK-HeaderTest-Exclude: FieldDisplacementMapData.h
