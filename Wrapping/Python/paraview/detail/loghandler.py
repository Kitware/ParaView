import logging

class VTKHandler(logging.Handler):
    def __init__(self, *args, **kwargs):
        super(VTKHandler, self).__init__(*args, **kwargs)

    def emit(self, record):
        try:
            from vtkmodules.vtkCommonCore import vtkLogger
            msg = self.format(record)
            vtkLogger.Log(\
                    self.get_vtk_level(record.levelno),
                    record.filename,
                    record.lineno,
                    msg)
        except Exception:
            self.handleError(record)

    def get_vtk_level(self, level):
        from vtkmodules.vtkCommonCore import vtkLogger
        if level >= logging.ERROR:
            return vtkLogger.VERBOSITY_ERROR
        elif level >= logging.WARNING:
            return vtkLogger.VERBOSITY_WARNING
        elif level >= logging.INFO:
            return vtkLogger.VERBOSITY_INFO
        elif level >= logging.DEBUG:
            return vtkLogger.VERBOSITY_TRACE
        else:
            return vtkLogger.VERBOSITY_MAX


def get_level(vtklevel=None):
    """returns current log level used by vtkLogger"""
    from vtkmodules.vtkCommonCore import vtkLogger
    vtk_level = vtkLogger.GetCurrentVerbosityCutoff() if vtklevel is None else vtklevel
    if vtk_level == vtkLogger.VERBOSITY_ERROR:
        return logging.ERROR
    elif vtk_level == vtkLogger.VERBOSITY_WARNING:
        return logging.WARNING
    elif vtk_level == vtkLogger.VERBOSITY_INFO:
        return logging.INFO
    else:
        return logging.DEBUG
