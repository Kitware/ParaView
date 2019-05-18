import paraview


class ProgressObject():
    '''Dummy progress reporter. See git history for how this used to work.'''
    def __init__(self):
        pass

    def UpdateProgress(self, progress):
        ''' 'progress' is expected to be in the range [0, 1]. '''
        pass

    def StartEvent(self):
        pass

    def EndEvent(self):
        pass
