/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlotEdges - 
// .SECTION Description


#ifndef __vtkPlotEdges_h
#define __vtkPlotEdges_h

#include "vtkMultiBlockDataSetAlgorithm.h"

//BTX
class Node;
class Segment;
class vtkPolyData;
class vtkCollection;
class vtkMultiBlockDataSet;
//ETX

class VTK_EXPORT vtkPlotEdges : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPlotEdges, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  static vtkPlotEdges *New();
//BTX
protected:
  vtkPlotEdges();
  virtual ~vtkPlotEdges();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, 
                  vtkInformationVector *);


  void Process(vtkPolyData* input, vtkMultiBlockDataSet* output);
  static void ReducePolyData(vtkPolyData* polyData, vtkPolyData* output);

  void ExtractSegments (vtkPolyData* polyData, vtkCollection* segments, 
                        vtkCollection* nodes);
  static void ExtractSegmentsFromExtremity (vtkPolyData* polyData,
                                            vtkCollection* segments,
                                            vtkCollection* nodes, 
                                            char* visitedCells,
                                            vtkIdType cellId,
                                            vtkIdType pointId,
                                            Node* node);
  static void ConnectSegmentsWithNodes (vtkCollection* segments,
                                        vtkCollection* nodes);
  static void SaveToMultiBlockDataSet (vtkCollection* segments,
                                       vtkMultiBlockDataSet* output);
  static void MergeSegments (vtkCollection* segments, vtkCollection* nodes,
                             Node* node, 
                             Segment* segmentA, Segment* segmentB);
  static Node* GetNodeAtPoint(vtkCollection* nodes, vtkIdType pointId);
  static void PrintSegments(vtkCollection* segments);

private:
  vtkPlotEdges(const vtkPlotEdges&);  // Not implemented.
  void operator=(const vtkPlotEdges&);  // Not implemented.
//ETX
};

#endif
