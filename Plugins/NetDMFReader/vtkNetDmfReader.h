/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetDmfReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNetDmfReader - read eXtensible Data Model and Format files
// .SECTION Description
// vtkNetDmfReader is a source object that reads XDMF data.
// The output of this reader is a vtkMultiGroupDataSet with one group for
// every enabled grid in the domain.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// uses the XDMF API
// .SECTION See Also
// vtkGraphReader

#ifndef __vtkNetDmfReader_h
#define __vtkNetDmfReader_h

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkDataObject;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkMutableDirectedGraph;
class vtkNetDmfReaderInternal;
class vtkNetDmfReaderGrid;

//BTX
class XdmfDsmBuffer;
class NetDMFDOM;
class NetDMFElement;
//ETX

class VTK_EXPORT vtkNetDmfReader : public vtkDirectedGraphAlgorithm
{
public:
  static vtkNetDmfReader* New();
  vtkTypeRevisionMacro(vtkNetDmfReader, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetFileName(const vtkStdString& fileName);
  //const vtkStdString& GetFileName()const;
  const char* GetFileName()const;
  
  // Description:
  // Determine if the file can be readed with this reader.
  virtual int CanReadFile(const char* fname);

  // Set the Timestep to be read. This is provided for compatibility
  // reasons only and should not be used. The correct way to
  // request time is using the UPDATE_TIME_STEPS information key
  // passed from downstream.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Save the range of valid timestep index values. This can be used by the PAraView GUI

  vtkGetVector2Macro(TimeStepRange, int);

  vtkSetMacro(ShowEvents, bool);
  vtkGetMacro(ShowEvents, bool);

  vtkSetMacro(ShowConversations, bool);
  vtkGetMacro(ShowConversations, bool);

  vtkSetMacro(ShowMovements, bool);
  vtkGetMacro(ShowMovements, bool);

  vtkSetMacro(ShowScenarios, bool);
  vtkGetMacro(ShowScenarios, bool);

  vtkSetMacro(ShowResults, bool);
  vtkGetMacro(ShowResults, bool);

  vtkSetMacro(ShowNodes, bool);
  vtkGetMacro(ShowNodes, bool);

  vtkSetMacro(ShowAddresses, bool);
  vtkGetMacro(ShowAddresses, bool);

protected:
  vtkNetDmfReader();
  ~vtkNetDmfReader();

  // Description:
  // This methods parses the XML. Returns true on success. This method can be
  // called repeatedly. It has checks to ensure that the XML parsing is done
  // only if needed.
  bool ParseXML();

  vtkStdString GetElementName(NetDMFElement* element);

  void AddNetDMFElement(vtkMutableDirectedGraph* graph,
                        NetDMFElement* element, 
                        vtkIdType parent = -1);

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  //virtual int FillOutputPortInformation(int port, vtkInformation *info);

  vtkNetDmfReaderInternal* Internal;

  unsigned int   ActualTimeStep;
  int            TimeStep;
  int            TimeStepRange[2];

  vtkTimeStamp   ParseTime;
  vtkStdString   FileName;
  long int       FileParseTime;

  bool           ShowEvents;
  bool           ShowConversations;
  bool           ShowMovements;
  bool           ShowScenarios;
  bool           ShowResults;
  bool           ShowNodes;
  bool           ShowAddresses;

private:
  vtkNetDmfReader(const vtkNetDmfReader&); // Not implemented
  void operator=(const vtkNetDmfReader&); // Not implemented
};

#endif //__vtkNetDmfReader_h
