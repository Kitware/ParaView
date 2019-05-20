r"""This module is used to export catalyst defined data products directly
within the ParaView GUI."""

from paraview.simple import *
haveCinemaC = True
try:
  import paraview.tpl.cinema_python.adaptors.paraview.pv_introspect as pvi
except:
  haveCinemaC = False
import os

class CinemaDHelper(object):
    """ A helper that we funnel file save commands through so that we can build up a CinemaD table for them. """
    def __init__(self, mcd, rd):
        self.__EnableCinemaDTable = mcd
        self.__RootDirectory = rd
        if rd is not '' and not rd.endswith("/"):
            self.__RootDirectory = rd + "/"
        self.Keys = set()
        self.Contents = []
        self.KeysWritten = None

    def __StripRootDir(self, filename):
        if self.__RootDirectory is not '':
            return filename[filename.startswith(self.__RootDirectory) and len(self.__RootDirectory):]
        return filename

    def __MakeCinDFileNamesUnderRootDir(self, filename=""):
        """ filename wrangling for root directory placement """
        indexfilename = 'data.csv'
        datafilename = filename
        if self.__RootDirectory is not '':
            indexfilename = self.__RootDirectory + "data.csv"
            # strip leading root directory from filename if present
            datafilename = self.__StripRootDir(filename)
        return indexfilename, datafilename

    def AppendToCinemaDTable(self, time, producer, filename):
        """ keep a record of every standard file written out so that we can list it later """
        if not self.__EnableCinemaDTable:
            return
        indexfilename, datafilename = self.__MakeCinDFileNamesUnderRootDir(filename)
        self.Keys.add("timestep")
        self.Keys.add("producer")
        self.Keys.add("FILE")
        self.Contents.append({'timestep':time,'producer':producer,'FILE':datafilename})

    def AppendCViewToCinemaDTable(self, time, producer, filelist):
        """ keep a record of every new file that cinema image writes along with the keys that produced them so that we can list them all later """
        if not self.__EnableCinemaDTable or not haveCinemaC:
            return
        self.Keys.add("timestep")
        self.Keys.add("producer")
        self.Keys.add("FILE")
        #unroll the contents into key lists and filenames
        for viewname in filelist:
           for entry in filelist[viewname]:
               keylist = entry[0]
               # avoid redundancy from name change
               if 'time' in keylist:
                   time = keylist['time']
                   del keylist['time']
               for k in keylist:
                   self.Keys.add(k)
               keylist['timestep']=time
               keylist['producer']=producer
               keylist['FILE']=self.__StripRootDir(entry[1])
               self.Contents.append(keylist)

    def Finalize(self):
        """ Finish off the table and write it to a file """
        # this is necessary because we don't really know keys (from cinema image) until the end.
        if not self.__EnableCinemaDTable:
            return
        indexfilename, datafilename = self.__MakeCinDFileNamesUnderRootDir()
        if self.__RootDirectory is not '' and not os.path.exists(self.__RootDirectory):
            os.makedirs(self.__RootDirectory)
        f = open(indexfilename, "w")
        # write the header line
        f.write("timestep,")
        f.write("producer,")
        for k in self.Keys:
            if k != 'timestep' and k != 'producer' and k != 'FILE':
                f.write("%s,"%k)
        f.write("FILE\n")
        # write all of the contents
        for l in self.Contents:
            f.write("%s,"%l['timestep'])
            f.write("%s,"%l['producer'])
            for k in self.Keys:
                if k != 'timestep' and k != 'producer' and k != 'FILE':
                    v = ''
                    if k in l:
                        v = l[k]
                    f.write("%s,"%v)
            f.write("%s\n"%self.__StripRootDir(l['FILE']))
        f.close()

    def WriteNow(self):
        """ For Catalyst, we don't generally have a Final state, so we call this every CoProcess call and fixup the table if we have to. """
        if not self.__EnableCinemaDTable:
            return

        indexfilename, datafilename = self.__MakeCinDFileNamesUnderRootDir()
        if self.KeysWritten == self.Keys:
            # phew nothing new, we can just append a record
            f = open(indexfilename, "a+")
            for l in self.Contents:
                f.write("%s,"%l['timestep'])
                f.write("%s,"%l['producer'])
                for k in self.Keys:
                    if k != 'timestep' and k != 'producer' and k != 'FILE':
                        v = ''
                        if k in l:
                            v = l[k]
                        f.write("%s,"%v)
                f.write("%s\n"%self.__StripRootDir(l['FILE']))
            f.close()
            self.Contents = []
            return
        #dang, whatever we wrote recently had a new variable
        #we may have to extend and rewrite the old output file
        readKeys = None
        readContents = []
        if os.path.exists(indexfilename):
            #yep we have to do it
            #parse the old file
            f = open(indexfilename, "r")
            for line in f:
                read = line[0:-1].split(",") #-1 to skip trailing\n
                if readKeys is None:
                    readKeys = read
                else:
                    entry = {}
                    for idx in range(0,len(read)):
                        entry.update({readKeys[idx]:read[idx]})
                    readContents.append(entry)
        #combine contents
        if readKeys is not None:
           self.Keys = self.Keys.union(readKeys)
           readContents.extend(self.Contents)
           self.Contents = readContents
        #finally, write
        self.Finalize()
        self.KeysWritten = self.Keys.copy()
        self.Contents = []


class __CinemaACHelper(object):
    """ Another helper that connects up to cinema_python's export function. """
    def __init__(self, rd, vsel, tsel, asel):
        self.__RootDirectory = rd
        self.__ViewSelection = vsel
        self.__TrackSelection = tsel
        self.__ArraySelection = asel
        self.NewFiles = None

    if haveCinemaC:
      def ExportNow(self, time):
          r = pvi.export_scene(baseDirName=self.__RootDirectory,
                           viewSelection=dict(self.__ViewSelection),
                           trackSelection=dict(self.__TrackSelection),
                           arraySelection=dict(self.__ArraySelection),
                           forcetime=time)
          self.NewFiles = r
    else:
      def ExportNow(self, time):
          pass

def ExportNow(root_directory,
              file_name_padding,
              make_cinema_table,
              cinema_tracks,
              cinema_arrays,
              rendering_info):
    """The user facing entry point. Here we get a hold of ParaView's animation controls, step through the animation, and export the things we've been asked to be the caller."""

    CIND = CinemaDHelper(make_cinema_table, root_directory)
    if root_directory is not '' and not root_directory.endswith("/"):
        root_directory = root_directory + "/"
    if root_directory is not '' and not os.path.exists(root_directory):
        os.makedirs(root_directory)

    # get a hold of the scene
    spm = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSessionProxyManager()
    ed = spm.GetExportDepot()
    s = GetAnimationScene()
    s.GoToFirst()
    et = s.EndTime
    tnow = s.AnimationTime

    numTimesteps = len(paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues)
    timesteps = {}
    # We must perform the export loop at least once, even in the case where
    # the input data contains no timesteps
    if (numTimesteps == 0):
        timesteps = list(range(1))
    else:
        timesteps = list(range(numTimesteps))

    # export loop
    for tstep in timesteps:
        padded_tstep = str(tstep).rjust(file_name_padding, '0')

        # loop through the configured writers and export at the requested times
        ed.InitNextWriterProxy()
        wp = ed.GetNextWriterProxy()
        writercnt = 0
        while wp:
            freq = wp.GetProperty("WriteFrequency").GetElement(0)
            if ((tstep % freq==0) and
                (not (wp.GetXMLName() == "Cinema image options"))):
                # this isn't pretty. I couldn't find a way to write
                # directly with the writer proxy. So what I do here
                # is find the input and filename and make a new writer
                # to use.
                proxyname = spm.GetProxyName("export_writers", wp)
                inputname = proxyname[0:proxyname.find("|")]
                inputproxy = FindSource(inputname)
                fname = wp.GetProperty("CatalystFilePattern").GetElement(0)
                if wp.GetXMLName() == "ExodusIIWriter":
                    fnamefilled = root_directory+fname+padded_tstep
                else:
                    fnamefilled = root_directory+fname.replace("%t", padded_tstep)

                DataMode = wp.GetProperty("DataMode")
                if DataMode is not None:
                    DataMode = wp.GetProperty("DataMode").GetElement(0)
                HeaderType = wp.GetProperty("HeaderType")
                if HeaderType is not None:
                    HeaderType = wp.GetProperty("HeaderType").GetElement(0)
                EncodeAppendedData=wp.GetProperty("EncodeAppendedData")
                if EncodeAppendedData is not None:
                    EncodeAppendedData = wp.GetProperty("EncodeAppendedData").GetElement(0)
                CompressorType = wp.GetProperty("CompressorType")
                if CompressorType is not None:
                    CompressorType = wp.GetProperty("CompressorType").GetElement(0)
                CompressionLevel = wp.GetProperty("CompressionLevel")
                if CompressionLevel is not None:
                    CompressionLevel = wp.GetProperty("CompressionLevel").GetElement(0)

                # finally after all of the finageling above, save the data
                SaveData(fnamefilled, inputproxy, DataMode=DataMode, HeaderType=HeaderType, EncodeAppendedData=EncodeAppendedData, CompressorType=CompressorType, CompressionLevel=CompressionLevel)
                # don't forget to tell cinema D about it
                CIND.AppendToCinemaDTable(tnow, "writer_%s" % writercnt, fnamefilled)
            wp = ed.GetNextWriterProxy()
            writercnt = writercnt + 1

        # loop through the configured screenshots and export at the requested times
        ed.InitNextScreenshotProxy()
        ssp = ed.GetNextScreenshotProxy()
        viewcnt = 0
        while ssp:
            freq = ssp.GetProperty("WriteFrequency").GetElement(0)
            if tstep % freq==0:
                fname = ssp.GetProperty("CatalystFilePattern").GetElement(0)
                if fname.endswith("cdb"):
                    CINAC = __CinemaACHelper(root_directory,
                              rendering_info,
                              cinema_tracks,
                              cinema_arrays)
                    # special treatment for cinema image data bases
                    CINAC.ExportNow(tnow)
                    # don't forget to tell cinema D about it
                    CIND.AppendCViewToCinemaDTable(tnow, "cview_%s"%viewcnt, CINAC.NewFiles)
                else:
                    fnamefilled = root_directory+fname.replace("%t", padded_tstep)
                    # save the screenshot
                    ssp.WriteImage(fnamefilled)
                    # don't forget to tell cinema D about it
                    CIND.AppendToCinemaDTable(tnow, "view_%s"%viewcnt, fnamefilled)
            ssp = ed.GetNextScreenshotProxy()
            viewcnt = viewcnt + 1
        tstep = tstep + 1
        s.GoToNext()
        tnow = s.AnimationTime

    # defer actual cinema D output until the end because we only know now what the full set of cinema D columns actually are
    CIND.Finalize()
