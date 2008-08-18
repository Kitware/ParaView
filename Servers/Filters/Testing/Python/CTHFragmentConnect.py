#!/usr/bin/python
import os
import sys
from paraview import servermanager as sm


class vtkCTHFragmentConnectTest:
  """Execute the CTH Fragment Connect filter on a
  set of given data file. Render the results."""
#{
  def __init__(self):
    """..."""
  #{
    self.DataSets=[]
    self.DataSetId=0
    self.topt={} # test options that are passed in via command tial
  #}

  def ProcessCommandLineArguments(self):
    """Processes the command line areguments.
    return a dictionary with the following keys:

    -D=/path/to/data/dir
    -T=/path/to/temp/dir
    -V=/path/to/validation/image with extention stripped
    -P=number of server processes
    --threshol=error threshold
    --genrate-base make a set of baseline images and data
    --no-test don't make comparisons with base line"""
  #{
    self.topt=dict([
          ('--generate-base',False),
          ('--threshold',10),
          ('--no-test',False),
          ('-D','./'),
          ('-T','./'),
          ('-V','./'),
          ('-P',1)])
    nArgs=len(sys.argv)
    #print nArgs
    i=1
    while(i<nArgs):
    #{
      arg=sys.argv[i]
      if(arg=="-D"):
      #{
        self.topt["-D"]=sys.argv[i+1]
        i+=1
      #}
      elif(arg=="-V"):
      #{
        self.topt["-V"]=os.path.splitext(sys.argv[i+1])[0]
        i+=1
      #}
      elif(arg=="-T"):
      #{
        self.topt["-T"]=sys.argv[i+1]
        i+=1
      #}
      elif(arg=="-P"):
      #{
        self.topt["-P"]=int(sys.argv[i+1])
        i+=1
      #}
      elif(arg=="--threshold"):
      #{
        self.topt["--thershold"]= float(sys.argv[i+1])
        i+=1
      #}
      elif(arg=="--generate-base"):
      #{
        self.topt["--generate-base"]=True
      #}
      elif(arg=="--no-test"):
      #{
        self.topt["--no-test"]=True
      #}
      # always skip
      i+=1
    #}
    #print self.topt
    return
  #}
  def Connect(self,serverMan):
    """Connect to the server. Must process the command tail first"""
  #{
    if(self.topt["-P"]>1):
    #{
      serverMan.Connect("localhost",11111)
    #}
    else:
    #{
      serverMan.Connect()
    #}
  #}

  def SetInputDataSets(self,files):
    """Set the list of file to process."""
  #{
    self.DataSets=files
    self.DataSetId=0
  #}

  def ExtractArrayNames(self,cellArrayInfo):
    """return a list with the array names from the 
    cell status array."""
  #{
    arrayNames=[]
    n=len(cellArrayInfo)/2
    i=0
    while (i<n):
    #{
      arrayNames.append(cellArrayInfo[2*i])
      i+=1
    #}
    return arrayNames
  #}

  def ExtractMassArrayNames(self,arrayNames):
    """Return a list of the arrays with Mass in there
    name."""
  #{
    massArrayNames=[]
    for name in arrayNames:
    #{
      if(name.find('Mass')==-1):
      #{
        continue;
      #}
      massArrayNames.append(name)
    #}
    return massArrayNames
  #}

  def ExtractMaterialFractionArrayNames(self,arrayNames):
    """Return a list of the arrays with Material fraction
    in there name."""
  #{
    matFrArrayNames=[]
    for name in arrayNames:
    #{
      if(name.find('fraction')==-1):
      #{
        continue;
      #}
      matFrArrayNames.append(name)
    #}
    return matFrArrayNames
  #}

  def ExtractWeightedAverageArrayNames(self,arrayNames):
    """Given a list of array names return a list
    with all names conatining Mass and fraction removed"""
  #{
    waaNames=[]
    for name in arrayNames:
    #{
      if (name.find('Mass')!=-1
          or name.find('fraction')!=-1
          or name.find('X ')==0
          or name.find('Y ')==0
          or name.find('Z ')==0
          or name.find('XX ')==0
          or name.find('XY ')==0
          or name.find('YY ')==0):
      #{
        continue;
      #}
      #vecIdx=name.find('X ')
      #if (vecIdx==0):
      ##{
        #waaNames.append(name[2:])
      ##}
      else:
      #{
        waaNames.append(name)
      #}
    #}
    #print waaNames
    return waaNames
  #}

  def EnableAllArrays(self,arrayNames):
    """Given a list of array names make a list with
    the names plus status ints to set to 1"""
  #{
    enabledArrays=[]
    for name in arrayNames:
    #{
      enabledArrays.append(name)
      enabledArrays.append('1')
    #}
    return enabledArrays
  #}

  def Execute(self,serverMan):
    """ Process all files we know about. Note:
    Call SetInputDataSets first"""
  #{
    if (len(self.DataSets)):
    #{
      for dataSet in self.DataSets:
      #{
        dataSet=os.path.join(self.topt["-D"],dataSet)
        print dataSet
        print "================================"\
              "================================"
        # configure the reader
        spy=serverMan.sources.spcthreader()
        spy.FileName=dataSet
        spy.MergeXYZComponents=1
        spy.UpdatePipelineInformation()
        arrays=self.ExtractArrayNames(spy.CellArrayInfo)
        massArrays=self.ExtractMassArrayNames(arrays)
        materialArrays=self.ExtractMaterialFractionArrayNames(arrays)
        averagedArrays=self.ExtractWeightedAverageArrayNames(arrays)
        #print "Mass Arrays: %s" % massArrays
        #print "Material Arrays: %s" % materialArrays
        #print "Weighted Averages: %s" % averagedArrays
        spy.CellArrayStatus=self.EnableAllArrays(arrays)
        # configure the fragment connect filter
        frag=serverMan.filters.CTHFragmentConnect(Input=spy)
        frag.SelectMassArray=massArrays
        frag.SelectMaterialArray=materialArrays
        frag.SelectVolumeWtdAvgArray=averagedArrays
        frag.SelectMassWtdAvgArray=averagedArrays
        frag.WriteStatisticsOutput=1
        frag.ComputeOBB=1
        # create the view
        vw=serverMan.CreateRenderView()
        serverMan.CreateRepresentation(frag,vw)
        #cam=vw.GetActiveCamera()
        #cam.Azimuth(22)
        #cam.Elevation(22)
        vw.StillRender()
        vw.ResetCamera()
        # run the filter on all time steps
        testId=0
        for stepTime in spy.TimestepValues:
        #{
          imgName=self.topt["-V"]+"."+str(testId)+".png"
          print stepTime
          print "================================"
          # Generate base line images and data.
          if (self.topt["--generate-base"]==True):
          #{
            frag.OutputBaseName=self.topt["-V"]+"."+str(testId)
            vw.ViewTime=stepTime
            vw.StillRender()
            vw.ResetCamera()
            vw.WriteImage(imgName,"vtkPNGWriter")
          #}
          # Do the regression test.
          elif(self.topt["--no-test"]==False):
          #{
            frag.OutputBaseName=self.topt["-T"]+os.path.split(self.topt["-V"])[1]+"."+str(testId)
            vw.ViewTime=stepTime
            vw.StillRender()
            # image regression test
            smt=serverMan.vtkSMTesting()
            smt.AddArgument("-T")
            smt.AddArgument(self.topt["-T"])
            smt.AddArgument("-V")
            smt.AddArgument(imgName)
            smt.SetRenderViewProxy(vw.SMProxy)
            tStat=smt.RegressionTest(self.topt["--threshold"])
            if (tStat!=1):
            #{
              eStr='CTH fragment connect image regression test failed '+dataSet+'@'+str(testId)+'.'
              raise ValueError, eStr
            #}
            # test stats output
            statsOutputFile=self.topt["-T"]+os.path.split(self.topt["-V"])[1]+"."+str(testId)+".cthfc.S"
            statsBaseLineFile=self.topt["-V"]+"."+str(testId)+".cthfc.S"
            tStat=os.system("diff %s %s"%(statsBaseLineFile, statsOutputFile))
            if (tStat!=0):
            #{
              eStr='CTH fragment connect integration test failed '+dataSet+'@'+str(testId)+'.'
              raise ValueError, eStr
            #}
          #}
          # Run without testing.
          else:
          #{
            vw.ViewTime=stepTime
            vw.StillRender()
            vw.ResetCamera()
          #}
          testId+=1
        #}
      #}
    #}
  #}

#}


# Run the test
dsf=["Data/spcth_fc_0.0"]
cfct=vtkCTHFragmentConnectTest()
cfct.ProcessCommandLineArguments()
cfct.Connect(sm)
cfct.SetInputDataSets(dsf)
cfct.Execute(sm)

