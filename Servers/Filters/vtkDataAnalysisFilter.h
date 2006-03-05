/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataAnalysisFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataAnalysisFilter - combined filter with picks/probes.
// .SECTION Description
// This filter is a combination of Pick/Probe. 
// Note that this filter collects all the data on the root node
// when running in multiprocess mode. This is since, it internally
// uses vtkPProbeFilter and vtkPPickFilter, both of which collect the 
// data on the root node.

#ifndef __vtkDataAnalysisFilter_h
#define __vtkDataAnalysisFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkAppendFilter;
class vtkMultiProcessController;
class vtkPickFilter;
class vtkPProbeFilter;

class VTK_EXPORT vtkDataAnalysisFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkDataAnalysisFilter* New();
  vtkTypeRevisionMacro(vtkDataAnalysisFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  void SetSource(vtkDataObject *source);
  vtkDataObject *GetSource();

  // Description:
  // Select whether you are picking point or cells. This works only
  // when mode is PICK.
  vtkSetMacro(PickCell,int);
  vtkGetMacro(PickCell,int);
  vtkBooleanMacro(PickCell,int);

  // Description:
  // Select whether we are using Ids to Pick. This works only when mode
  // is PICK. When 0, the input dataset is used as the geometry to pick at.
  // When 1, the Id is used: if PickCell is 0, the Id is interpreted as 
  // point id, when PickCell is 1, Id is interpreted as cell id.
  vtkSetMacro(UseIdToPick,int);
  vtkGetMacro(UseIdToPick,int);
  vtkBooleanMacro(UseIdToPick,int);
  
  // Description:
  // If using an Id to pick, set the ID with this method. Works only
  // when mode is PICK.
  vtkSetMacro(Id,vtkIdType);
  vtkGetMacro(Id,vtkIdType);
  
  // Descrption:
  // If the input point/cell attributes has an array with this name,
  // then it is used to find the point.  Defaults to GlobalId.
  // Works only when mode is PICK.
  vtkSetStringMacro(GlobalPointIdArrayName);
  vtkGetStringMacro(GlobalPointIdArrayName);
  vtkSetStringMacro(GlobalCellIdArrayName);
  vtkGetStringMacro(GlobalCellIdArrayName);
  
  // Description:
  // This flag is used only when a piece is requested to update.  By default
  // the flag is off.  Because no spatial correspondence between input pieces
  // and source pieces is known, all of the source has to be requested no
  // matter what piece of the output is requested.  When there is a spatial 
  // correspondence, the user/application can set this flag.  This hint allows
  // the breakup of the probe operation to be much more efficient.  When piece
  // m of n is requested for update by the user, then only n of m needs to
  // be requested of the source. This is used only when mode is PROBE.
  vtkSetMacro(SpatialMatch, int);
  vtkGetMacro(SpatialMatch, int);
  vtkBooleanMacro(SpatialMatch, int);
  
  // Description:
  // This is set by default (if compiled with MPI).
  // User can override this default.
  void SetController(vtkMultiProcessController* controller);

  //BTX
  enum FilterModes {
    PROBE,
    PICK
  };
  //ETX

  // Description:
  // Get/Set the world point to pick at.
  // Use only when mode is PICK and UseIdToPick is false.
  vtkSetVector3Macro(WorldPoint, double);
  vtkGetVector3Macro(WorldPoint, double);

  // Description:
  // Get/Set the mode of operation. Indicates if the filter behaves as a pick
  // or a probe. Default is PROBE.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
protected:
  vtkDataAnalysisFilter();
  ~vtkDataAnalysisFilter();

  int Mode;
  int SpatialMatch;
  int PickCell;
  int UseIdToPick;
  double WorldPoint[3];
  vtkIdType Id;
  char* GlobalPointIdArrayName;
  char* GlobalCellIdArrayName;
  vtkMultiProcessController* Controller;

  vtkPProbeFilter* ProbeFilter;
  vtkPickFilter* PickFilter;
  vtkAppendFilter* DataSetToUnstructuredGridFilter;

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Declare the types of input data.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkDataAnalysisFilter(const vtkDataAnalysisFilter&); // Not implemented.
  void operator=(const vtkDataAnalysisFilter&); // Not implemented.
};


#endif

