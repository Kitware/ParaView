/*=========================================================================

  Visualization and Analysis Platform for Ocean, Atmosphere, and Solar
  Researchers (VAPOR)
  Terms of Use

  This VAPOR (the Software ) was developed by the University
  Corporation for Atmospheric Research.

  PLEASE READ THIS SOFTWARE LICENSE AGREEMENT ("AGREEMENT") CAREFULLY.
  INDICATE YOUR ACCEPTANCE OF THESE TERMS BY SELECTING THE  I ACCEPT
  BUTTON AT THE END OF THIS AGREEMENT. IF YOU DO NOT AGREE TO ALL OF THE
  TERMS OF THIS AGREEMENT, SELECT THE  I DON?T ACCEPT? BUTTON AND THE
  INSTALLATION PROCESS WILL NOT CONTINUE.

  1.      License.  The University Corporation for Atmospheric Research
  (UCAR) grants you a non-exclusive right to use, create derivative
  works, publish, distribute, disseminate, transfer, modify, revise and
  copy the Software.

  2.      Proprietary Rights.  Title, ownership rights, and intellectual
  property rights in the Software shall remain in UCAR.

  3.      Disclaimer of Warranty on Software. You expressly acknowledge
  and agree that use of the Software is at your sole risk. The Software
  is provided "AS IS" and without warranty of any kind and UCAR
  EXPRESSLY DISCLAIMS ALL WARRANTIES AND/OR CONDITIONS OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTIES OR
  CONDITIONS OF TITLE, NON-INFRINGEMENT OF A THIRD PARTY?S INTELLECTUAL
  PROPERTY, MERCHANTABILITY OR SATISFACTORY QUALITY AND FITNESS FOR A
  PARTICULAR PURPOSE. UCAR DOES NOT WARRANT THAT THE FUNCTIONS CONTAINED
  IN THE SOFTWARE WILL MEET YOUR REQUIREMENTS, OR THAT THE OPERATION OF
  THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT DEFECTS IN
  THE SOFTWARE WILL BE CORRECTED. FURTHERMORE, UCAR DOES NOT WARRANT OR
  MAKE ANY REPRESENTATIONS AND YOU ASSUME ALL RISK REGARDING THE USE OR
  THE RESULTS OF THE USE OF THE SOFTWARE OR RELATED DOCUMENTATION IN
  TERMS OF THEIR CORRECTNESS, ACCURACY, RELIABILITY, OR OTHERWISE. THE
  PARTIES EXPRESSLY DISCLAIM THAT THE UNIFORM COMPUTER INFORMATION
  TRANSACTIONS ACT (UCITA) APPLIES TO OR GOVERNS THIS AGREEMENT.  No
  oral or written information or advice given by UCAR or a UCAR
  authorized representative shall create a warranty or in any way
  increase the scope of this warranty. Should the Software prove
  defective, you (and not UCAR or any UCAR representative) assume the
  cost of all necessary correction.

  4.      Limitation of Liability.  UNDER NO CIRCUMSTANCES, INCLUDING
  NEGLIGENCE, SHALL UCAR OR ITS COLLABORATORS, INCLUDING OHIO STATE
  UNIVERSITY, BE LIABLE FOR ANY DIRECT, INCIDENTAL, SPECIAL, INDIRECT OR
  CONSEQUENTIAL DAMAGES INCLUDING LOST REVENUE, PROFIT OR DATA, WHETHER
  IN AN ACTION IN CONTRACT OR TORT ARISING OUT OF OR RELATING TO THE USE
  OF OR INABILITY TO USE THE SOFTWARE, EVEN IF UCAR HAS BEEN ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGES.

  5.      Compliance with Law. All Software and any technical data
  delivered under this Agreement are subject to U.S. export control laws
  and may be subject to export or import regulations in other countries.
  You agree to comply strictly with all applicable laws and regulations
  in connection with use and distribution of the Software, including
  export control laws, and you acknowledge that you have responsibility
  to obtain any required license to export, re-export, or import as may
  be required.

  6.      No Support/Modifications. The names UCAR/NCAR and SCD may not
  be used in any advertising or publicity to endorse or promote any
  products or commercial entity unless specific written permission is
  obtained from UCAR.  The Software is provided without any support or
  maintenance, and without any obligation to provide you with
  modifications, improvements, enhancements, or updates of the Software.

  7.      Controlling Law and Severability.  This Agreement shall be
  governed by the laws of the United States and the State of Colorado.
  If for any reason a court of competent jurisdiction finds any
  provision, or portion thereof, to be unenforceable, the remainder of
  this Agreement shall continue in full force and effect. This Agreement
  shall not be governed by the United Nations Convention on Contracts
  for the International Sale of Goods, the application of which is
  hereby expressly excluded.

  8.      Termination.  Your rights under this Agreement will terminate
  automatically without notice from UCAR if you fail to comply with any
  term(s) of this Agreement.   You may terminate this Agreement at any
  time by destroying the Software and any related documentation and any
  complete or partial copies thereof.  Upon termination, all rights
  granted under this Agreement shall terminate.  The following
  provisions shall survive termination: Sections 2, 3, 4, 7 and 10.

  9.      Complete Agreement. This Agreement constitutes the entire
  agreement between the parties with respect to the use of the Software
  and supersedes all prior or contemporaneous understandings regarding
  such subject matter. No amendment to or modification of this Agreement
  will be binding unless in a writing and signed by UCAR.

  10.  Notices and Additional Terms.  Each copy of the Software shall
  include a copy of this Agreement and the following notice:

  "The source of this material is the Science Computing Division of the
  National Center for Atmospheric Research, a program of the University
  Corporation for Atmospheric Research (UCAR) pursuant to a Cooperative
  Agreement with the National Science Foundation; Copyright (c)2006 University
  Corporation for Atmospheric Research. All Rights Reserved."

  This notice shall be displayed on any documents, media, printouts, and
  visualizations or on any other electronic or tangible expressions
  associated with, related to or derived from the Software or associated
  documentation.

  The Software includes certain copyrighted, segregable components
  listed below (the Third Party Code?), including software developed by
  Ohio State University.  For this reason, you must check the source
  identified below for additional notice requirements and terms of use
  that apply to your use of this Software.

--------------------------------------------------------------------------

  Program:   Visualization Toolkit
  Module:    vtkVDFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVDFReader - class for reader Vapor Data Format files
// .Section Description
// vtkVDFReader uses the Vapor library to read wavelet compressed VDF files
// and produce VTK data image data. The resolution of the image data is
// controlled by the refinement parameter.
//

#ifndef vtkVDFReader_h
#define vtkVDFReader_h

#include "vtkImageAlgorithm.h"
#include "vtkVaporReadersModule.h" // for export macro

#include "vapor/DataMgr.h"                    //needed for vapor datastructures
#include "vapor/WaveletBlock3DRegionReader.h" //needed for vapor datastructures

#include <map>    //needed for protected ivars
#include <string> //needed for protected ivars
#include <vector> //needed for protected ivars

// using namespace std;
using namespace VAPoR;

class VTKVAPORREADERS_EXPORT vtkVDFReader : public vtkImageAlgorithm
{
public:
  static vtkVDFReader* New();
  vtkTypeMacro(vtkVDFReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Choose file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Check file for suitability to this reader
  int CanReadFile(const char* fname);

  // Description:
  // Set resolution within range provided by data.
  void SetRefinement(int);
  vtkGetMacro(Refinement, int);
  vtkGetVector2Macro(RefinementRange, int);

  // Description:
  void SetVariableType(int);
  vtkGetMacro(VariableType, int);

  // Description:
  void SetCacheSize(int);
  vtkGetMacro(CacheSize, int);

  // Description:
  // Choose which arrays to load
  int GetPointArrayStatus(const char*);
  void SetPointArrayStatus(const char*, int);
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int);

protected:
  vtkVDFReader();
  ~vtkVDFReader();
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillTimeSteps();

  char* FileName;

  Metadata* vdc_md;
  DataMgr* data_mgr;
  VDFIOBase* vdfiobase;

  bool is_layered;
  int height_factor;
  int num_levels;
  size_t ext_p[3];
  int Refinement;
  int VariableType;
  int CacheSize;
  double* TimeSteps;
  int nTimeSteps;
  int TimeStep;
  int RefinementRange[2];

  std::map<std::string, int> data;
  std::vector<double> uExt;
  std::vector<std::string> current_var_list;

private:
  vtkVDFReader(const vtkVDFReader&);
  void operator=(const vtkVDFReader&);
  bool extentsMatch(int*, int*);
  void GetVarDims(size_t*, size_t*);
  void SetExtents(vtkInformation*);
};
#endif
