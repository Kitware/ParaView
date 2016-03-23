r"""
Internal module used by paraview.servermanager to help warn about properties
changed or removed.

If the compatibility version is less than the version where a particular
property was removed, `check_attr` should ideally continue to work as before
or return a value of appropriate form so old code doesn't fail. Otherwise
`check_attr` should throw the NotSupportedException with appropriate debug
message.
"""

import paraview

class NotSupportedException(Exception):
    def __init__(self, msg):
        self.msg = msg
        paraview.print_debug_info("\nDEBUG: %s\n" % msg)

class Continue(Exception):
    pass

def getattr(proxy, pname):
    """
    Attempts to emulate getattr() when called using a deprecated property name
    for a proxy.

    Will return a *resonable* standin if the property was
    deprecated and the paraview compatibility version was set to a version older
    than when the property was deprecated.

    Will raise ``NotSupportedException`` if the property was deprecated and
    paraview compatibility version is newer than that deprecation version.

    Will raise ``Continue`` to indicate the property name is unaffected by
    any API deprecation and the caller should follow normal code execution
    paths.
    """
    version = paraview.compatibility.GetVersion()

    # In 4.2, we removed ColorAttributeType property. One is expected to use
    # ColorArrayName to specify the attribute type as well.
    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if version <= 4.1:
            if proxy.GetProperty("ColorArrayName")[0] == "CELLS":
                return "CELL_DATA"
            else:
                return "POINT_DATA"
        else:
            # if ColorAttributeType is being used, warn.
            raise NotSupportedException(
                "'ColorAttributeType' is obsolete. Simply use 'ColorArrayName' instead.  Refer to ParaView Python API changes documentation online.")

    # In 5.1, we removed CameraClippingRange property. It was not of any use
    # since we added support to render view to automatically reset clipping
    # range for each render.
    if pname == "CameraClippingRange" and not proxy.SMProxy.GetProperty("CameraClippingRange"):
        if version <= 5.0:
            return [0.0, 0.0, 0.0]
        else:
            raise NotSupportedException(
                    'CameraClippingRange is obsolete. Please remove '\
                    'it from your script. You no longer need it.')
    raise Continue()
