r"""
   This module provides classes to handle Rest API for WebGL piece request.

   This module is now deprecated.
"""

from twisted.web import server, resource
from twisted.web import server, resource
from twisted.web.error import Error

from paraview import simple
from paraview.web import helper
from vtk.vtkParaViewWebCore import vtkPVWebApplication

import exceptions
import base64

pv_web_app = vtkPVWebApplication()

class WebGLResource(resource.Resource):
    """
    Resource using to serve WebGL data. If supports URLs of two forms:
    - to get the meta-data as json-rpc message:
           http://.../WebGL?q=meta&vid=456

        - to get a binary webgl object:
           http://.../WebGL?q=mesh&vid=456&id=1&part=0

           vid : visualization view ID    (GlobalID of the view Proxy)
           id  : id of the object in the scene
           part: WebGL has a size limit, therefore an object can be splitted in
           several part. This is the part index.
    """
    def _missing_parameter(self, parameter):
        raise Error(400, "Missing required parameter: %s" % parameter)

    def _get_parameter(self, args, parameter, required=True):
        """
        Extract a parameter value from the request args.
        :param parameter: The parameter name
        :type parameter: str
        :param required: True if the function should raise an error is the
        parameter is missing.
        :type required: bool
        """
        value = None

        if not parameter in args:
            if required:
                self._missing_parameter(parameter)
        else:
            values = args[parameter]
            if len(values) == 1:
                value = values[0]
            elif required:
               self._missing_parameter(parameter)

        return value

    def _get_view(self, vid):
        """
        Returns the view for a given view ID, if vid is None then return the
        current active view.
        :param vid: The view ID
        :type vid: str
        """
        def _get_active_view():
            view = simple.GetActiveView()
            if not view:
                raise Error(404, "No view provided to WebGL resource")
            return view

        view = None

        if vid:
            try:
                view = helper.mapIdToProxy(vid)
            except exceptions.TypeError:
                pass

        if not view:
            view = _get_active_view()

        return view

    def _render_GET_mesh(self, vid, request):
        """
        Handle GET requests to get WebGL data for a particular object.
        """
        object_id =  self._get_parameter(request.args, 'id')
        part_number = self._get_parameter(request.args, 'part', False);

        part = 0
        if part_number:
            part = int(part_number)

        view = self._get_view(vid)

        # There part is offset by 1
        if part > 0:
            part = part - 1

        data = pv_web_app.GetWebGLBinaryData(view.SMProxy, object_id, part);

        if data:
            request.setHeader('content-type', 'application/octet-stream+webgl')
            request.setHeader('Cache-Control', 'max-age=99999');
            data = base64.b64decode(data)
        else:
           raise Error(404, "Invalid request for WebGL object")

        return data

    def _render_GET_meta(self, vid, request):
        """
        Handle GET request for meta data.
        """
        view = self._get_view(vid)

        data = pv_web_app.GetWebGLSceneMetaData(view.SMProxy)

        if data:
            request.setHeader('content-type', 'application/json')
        else:
            raise Error(404, "Invalid request for WebGL meta data")

        return data

    def render_GET(self, request):
        """
        Handle GET requests, parse out q parameter to delegate to the correct
        internal function.
        There is two types of queries allowed:
        - to get the meta-data as json-rpc message:
           http://.../WebGL?q=meta&vid=456

        - to get a binary webgl object:
           http://.../WebGL?q=mesh&vid=456&id=1&part=0

           vid : visualization view ID    (GlobalID of the view Proxy)
           id  : id of the object in the scene
           part: WebGL has a size limit, therefore an object can be splitted in several part. This is the part index.
        """
        try:
            q = self._get_parameter(request.args, 'q')
            vid = self._get_parameter(request.args, 'vid', False)

            if q == 'mesh':
                return self._render_GET_mesh(vid, request)
            elif q == 'meta':
                return self._render_GET_meta(vid, request)
            else:
                raise Error(400, "Invalid query for the WebGL resource");
        except Error as err:
            request.setResponseCode(err.status)
            return err.message
