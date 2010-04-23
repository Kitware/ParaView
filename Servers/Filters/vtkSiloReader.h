/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSiloReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

 Note: Parts of the code found in this file are verbatim VisIt source code
 from the VisIt source files avtSiloFileFormat.C and avtSiloFileFormat.h.
 The authors of these files include:

    Kathleen Bonnell
    Eric Brugger
    Hank Childs
    Cyrus Harrison
    Jeremy Meredith
    Mark C. Miller

 The following copyright notice is included with avtSiloFileFormat...

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

// .NAME vtkSiloReader - Silo data set reader
// .SECTION Description
// vtkSiloReader reads a Silo data set and outputs a multiblock dataset.


#ifndef __vtkSiloReader_h
#define __vtkSiloReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "silo.h" // Needed for Silo structs
// Silo structs are anonymous structs so they
// cannot be forward declared.  A vtkSiloReaderHelper
// class will be written so that the Silo structs
// can be removed from this header.


class vtkDataSet;
class vtkDataArray;
class vtkUnstructuredGrid;
class vtkMultiProcessController;
class vtkMultiBlockDataSet;
class vtkSiloReaderHelper;
class vtkSiloTableOfContents;

struct SiloReaderInternals;

class VTK_EXPORT vtkSiloReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkSiloReader *New();
  vtkTypeMacro(vtkSiloReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetFileName(char *);
  vtkGetStringMacro(FileName);
  const char * GetTOCString();

protected:
  vtkSiloReader();
  ~vtkSiloReader();

  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

private:

  char *FileName;
  bool TableOfContentsRead;
  int TableOfContentsFileIndex;
  int ProcessId;
  int NumProcesses;
  static bool MadeGlobalSiloCalls;
  vtkSiloReaderHelper *Helper;
  SiloReaderInternals *Internals;
  vtkSiloTableOfContents *TOC;
  vtkMultiProcessController *Controller;

  int FileIsInDomain(int);
  void BroadcastTableOfContents();
  void TestBroadcastTableOfContents();
  int ReadTableOfContents();
  void RegisterDomainDirs(char **, int, const char*);
  bool ShouldGoToDir(const char *);
  void ReadDir(DBfile *, const char *);
  int GetMeshType(DBfile *, char *);
  int CreateDataSet(vtkMultiBlockDataSet *);
  int VerifyQuadmesh(DBquadmesh *, const char *);
  void GetQuadGhostZones(DBquadmesh *, vtkDataSet *);
  void ReadInConnectivity(vtkUnstructuredGrid *,DBzonelist *, int);
  void DetermineFileAndDirectory(char *, DBfile *&, const char *, char *&, bool *alloc=0);
  void DetermineFileIndexAndDirectory(char *, int &, const char *, char *&, bool *alloc=0);
  void DetermineFilenameAndDirectory(char *, const char *, char *, char *&, bool *alloc=0);
  void GetRelativeVarName(const char *,const char *,char *);
  vtkDataSet *GetPointMesh(DBfile *, const char *);
  vtkDataSet *GetQuadMesh(DBfile *, const char *);
  vtkDataSet *GetCurve(DBfile *, const char *);
  vtkDataSet *GetUnstructuredMesh(DBfile *, const char *);
  vtkDataSet *CreateRectilinearMesh(DBquadmesh *);
  vtkDataSet *CreateCurvilinearMesh(DBquadmesh *);
  vtkDataArray *GetMaterialArray(DBfile *, const char *, int);
  vtkDataArray *GetPointVar(DBfile *, const char *, int &);
  vtkDataArray *GetPointScalarVar(DBmeshvar *);
  vtkDataArray *GetPointVectorVar(DBmeshvar *);
  vtkDataArray *GetQuadVar(DBfile *, const char *, int &);
  vtkDataArray *GetQuadScalarVar(DBquadvar *);
  vtkDataArray *GetQuadVectorVar(DBquadvar *);
  vtkDataArray *GetUcdVar(DBfile *, const char *, int &);
  vtkDataArray *GetUcdScalarVar(DBucdvar *);
  vtkDataArray *GetUcdVectorVar(DBucdvar *);

  vtkSiloReader(const vtkSiloReader&);  // Not implemented.
  void operator=(const vtkSiloReader&);  // Not implemented.
};

#endif // __vtkSiloReader_h
