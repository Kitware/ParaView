r"""internal module used to get the list of default proxies.

This is an internal module and not intended to be used by Python code outside of
this package.

"""

__defaultProxiesJSON = """
{
    "sources": [
        { "name": "AnnotateTime", "label": "Annotate Time" },
        { "name": "Cone" },
        { "name": "Sphere" },
        { "name": "Text" },
        { "name": "Wavelet" }
    ],

    "filters": [
        { "name": "Calculator" },
        { "name": "CellDatatoPointData", "label": "Cell Data To Point Data" },
        { "name": "Clip" },
        { "name": "Contour" },
        { "name": "D3" },
        { "name": "ExtractCTHParts", "label": "Extract CTH Parts" },
        { "name": "ProcessIdScalars", "label": "Process ID Scalars" },
        { "name": "Reflect" },
        { "name": "Slice" },
        { "name": "StreamTracer", "label": "Stream Tracer" },
        { "name": "Threshold" },
        { "name": "Transform" },
        { "name": "Tube" },
        { "name": "Ribbon" },
        { "name": "WarpByScalar", "label": "Warp By Scalar" },
        { "name": "WarpByVector", "label": "Warp By Vector" },
        { "name": "ExtractBlock", "label": "Extract Blocks" }
    ],

    "readers": [
        { "name": "LegacyVTKReader", "extensions": [ "vtk" ], "method": "FileNames" },
        { "name": "Xdmf3ReaderS", "extensions": [ "xmf", "xdmf" ] }
    ]
}
"""

def getDefaultProxies():
    """Returns the JSON object for the default proxies configuration"""
    import json
    return json.loads(__defaultProxiesJSON)
