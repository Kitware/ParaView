/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVServerInformation
 * @brief   Gets features of the server.
 *
 * This objects is used by the client to get the features
 * supported by the server.
 * At the moment, server information is only on the root.
*/

#ifndef vtkPVServerInformation_h
#define vtkPVServerInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkClientServerStream;
class vtkPVServerOptionsInternals;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVServerInformation : public vtkPVInformation
{
public:
  static vtkPVServerInformation* New();
  vtkTypeMacro(vtkPVServerInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * This flag indicates whether the server can render remotely.
   * If it is off, all rendering has to be on the client.
   * This is only off when the user starts the server with
   * the --disable-composite command line option.
   */
  vtkSetMacro(RemoteRendering, int);
  vtkGetMacro(RemoteRendering, int);
  //@}

  void DeepCopy(vtkPVServerInformation* info);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Variables (command line argurments) set to render to a tiled display.
   */
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);
  //@}

  //@{
  /**
   * Returns 1 if IceT is available.
   */
  vtkSetMacro(UseIceT, int);
  vtkGetMacro(UseIceT, int);
  //@}

  //@{
  /**
   * if the server supports compressing images via NVPipe
   */
  //@}
  vtkSetMacro(NVPipeSupport, bool);
  vtkGetMacro(NVPipeSupport, bool);

  //@{
  /**
   * Get/Set the time after which the server timesout.
   */
  vtkSetMacro(Timeout, int);
  vtkGetMacro(Timeout, int);
  //@}

  //@{
  /**
   * Set/Get the EyeSeparation on server
   */
  void SetEyeSeparation(double value);
  double GetEyeSeparation() const;
  //@}

  //@{
  /**
   * Number of machines to use in data or render server
   * Setting the number of machines has the side effect of wiping out any
   * machine parameters previously set.
   */
  void SetNumberOfMachines(unsigned int num);
  unsigned int GetNumberOfMachines() const;
  //@}

  //@{
  /**
   * Value of DISPLAY environment variable for this cave node
   */
  void SetEnvironment(unsigned int idx, const char* name);
  const char* GetEnvironment(unsigned int idx) const;
  //@}

  //@{
  /**
   * Window geometry for server, specified as x, y, width, height. This is only
   * used if FullScreen is false.
   */
  void SetGeometry(unsigned int idx, int geo[4]);
  int* GetGeometry(unsigned int idx) const;
  //@}

  //@{
  /**
   * Whether to show the server window as fullscreen.
   */
  void SetFullScreen(unsigned int idx, bool fullscreen);
  bool GetFullScreen(unsigned int idx) const;
  //@}

  //@{
  /**
   * Whether to show the server window with window decorations.
   */
  void SetShowBorders(unsigned int idx, bool borders);
  bool GetShowBorders(unsigned int idx) const;
  //@}

  //@{
  /**
   * Get the stereo-type specified in the pvx. -1=no-specified, 0=no-stereo.
   */
  int GetStereoType(unsigned int idx) const;
  void SetStereoType(unsigned int idx, int type);
  //@}

  //@{
  /**
   * Coordinates of lower left corner of this cave display
   */
  void SetLowerLeft(unsigned int idx, double coord[3]);
  double* GetLowerLeft(unsigned int idx) const;
  //@}

  //@{
  /**
   * Coordinates of lower right corner of this cave display
   */
  void SetLowerRight(unsigned int idx, double coord[3]);
  double* GetLowerRight(unsigned int idx) const;
  //@}

  //@{
  /**
   * Coordinates of lower left corner of this cave display
   */
  void SetUpperRight(unsigned int idx, double coord[3]);
  double* GetUpperRight(unsigned int idx) const;
  //@}

  //@{
  /**
   * Get the number of processes.
   */
  vtkGetMacro(NumberOfProcesses, int);
  //@}

  /**
   * Return whether MPI is initialized or not.
   */
  virtual bool IsMPIInitialized() const;

  //@{
  /**
   * Return true if the server allow server client to connect to itself
   */
  vtkGetMacro(MultiClientsEnable, int);
  //@}

  //@{
  /**
   * Get the id that correspond to the current client
   */
  vtkGetMacro(ClientId, int);
  //@}

  //@{
  /**
   * Set/Get vtkIdType size, which can be 32 or 64
   */
  vtkSetMacro(IdTypeSize, int);
  vtkGetMacro(IdTypeSize, int);
  //@}

protected:
  vtkPVServerInformation();
  ~vtkPVServerInformation() override;

  int NumberOfProcesses;
  bool MPIInitialized;
  int OGVSupport;
  int AVISupport;
  bool NVPipeSupport;
  int RemoteRendering;
  int TileDimensions[2];
  int TileMullions[2];
  int Timeout;
  int UseIceT;
  int MultiClientsEnable;
  int ClientId;
  int IdTypeSize;

  vtkPVServerOptionsInternals* MachinesInternals;

  vtkPVServerInformation(const vtkPVServerInformation&) = delete;
  void operator=(const vtkPVServerInformation&) = delete;
};

#endif
