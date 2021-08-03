from paraview.simple import *
from paraview import print_info, print_error


def GetNumberOfRanks():
    session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
    return session.GetNumberOfProcesses(session.DATA_SERVER)

def GetRank():
    return servermanager.vtkProcessModule.GetProcessModule().GetPartitionId()

def GetIsSymmetric():
    return servermanager.vtkProcessModule.GetSymmetricMPIMode()

def ValidateData(producer, data):
    return data.IsA("vtkImageData")

def TestFetchDataBasic(producer, allGather=False):
    print_info("TestFetchDataBasic")
    dataMap = FetchData(producer, GatherOnAllRanks = allGather)

    if not allGather and GetIsSymmetric() and GetRank() > 0:
        # dataMap must be empty.
        if dataMap:
            print_error("FetchData should not deliver any data on satellites!")
            return False
    else:
        dataRanks = [x for x in dataMap.keys()]

        # ensure we got data from all rank.
        numRanks = GetNumberOfRanks()

        if len(dataRanks) != numRanks:
            print_error("rank mismatch len(%s) != %d", repr(dataRanks), numRanks)
            return False

        # ensure we got valid data from all ranks
        for rank, data in dataMap.items():
            if not ValidateData(producer, data):
                print_error("bad data from rank %d", rank)
                return False

    return True

def TestFetchDataRanks(producer, ranks, allGather = False):
    print_info("TestFetchDataRanks %s" % repr(ranks))
    dataMap = FetchData(producer, SourceRanks=ranks, GatherOnAllRanks = allGather)

    if not allGather and GetIsSymmetric() and GetRank() > 0:
        # dataMap must be empty.
        if dataMap:
            print_error("FetchData should not deliver any data on satellites!")
            return False
    else:
        dataRanks = [x for x in dataMap.keys()]
        expectedRanks = []

        numRanks = GetNumberOfRanks()
        for r in ranks:
            if r < numRanks:
                expectedRanks.append(r)

        if dataRanks != expectedRanks:
            print_error("ranks mismatch %s != %s", repr(dataRanks), repr(expectedRanks))
            return False

        # ensure we got valid data from all ranks
        for rank, data in dataMap.items():
            if not ValidateData(producer, data):
                print_error("bad data from rank %d", rank)
                return False
    return True

if __name__ == "__main__":
    wavelet = Wavelet()
    UpdatePipeline()
    if not TestFetchDataBasic(wavelet):
        raise RuntimeError("TestFetchDataBasic failed")
    if not TestFetchDataRanks(wavelet, [0]):
        raise RuntimeError("TestFetchDataRanks([0]) failed")
    if not TestFetchDataRanks(wavelet, [1,2]):
        raise RuntimeError("TestFetchDataRanks([1,2]) failed")
    if GetIsSymmetric():
      print_info("In symmetric mode, so try gather on all ranks now")
      if not TestFetchDataBasic(wavelet, True):
          raise RuntimeError("TestFetchDataBasic(allGather = True) failed")
      if not TestFetchDataRanks(wavelet, [0], True):
          raise RuntimeError("TestFetchDataRanks([0], allGather=True) failed")
      if not TestFetchDataRanks(wavelet, [1,2], True):
          raise RuntimeError("TestFetchDataRanks([1,2], allGather=True) failed")
