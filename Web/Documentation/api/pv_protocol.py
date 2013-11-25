// This file is used to generate online documentation for ParaView protocols.

/**
 * @class server.paraview.web.ParaViewWebProtocol
 *
 * Base protocol class on the server side for ParaViewWeb application.
 *
 * In order to call a protocol method from the client side you will need
 * to use the call method on the session with the name of the method like
 * shown below.
 *
 *      session.call("vtk:methodName", arg1, arg2).then(function(response) {
 *          // Can process the response now
 *      });
 */

// =====================================================================

/**
 * @class server.paraview.web.ParaViewWebMouseHandler
 *
 * This protocol handle mouse interaction requests.
 */

/**
 * @member server.paraview.web.ParaViewWebMouseHandler
 * @method mouseInteraction
 * @param {Object} event
 *
 *     event = {
 *        buttonLeft: true,
 *        buttonMiddle: false,
 *        buttonRight: false,
 *        shiftKey: false,
 *        ctrlKey: false,
 *        altKey: false,
 *        metaKey: false,
 *        x: 0.4,         // Normalized x coordinate
 *        y: 0.1,         // Normalized y coordinate
 *        action: "down", // Enum["down", "up", "move", "dblclick", "scroll"]
 *        charCode: "",   // In key press will hold the char value
 *     }
 */

// =====================================================================

/**
 * @class server.paraview.web.ParaViewWebViewPort
 *
 * This protocol handle view common methods.
 */

/**
 * @member server.paraview.web.ParaViewWebViewPort
 * @method resetCamera
 * @param {String} viewId
 */

/**
 * @member server.paraview.web.ParaViewWebViewPort
 * @method updateOrientationAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 */

/**
 * @member server.paraview.web.ParaViewWebViewPort
 * @method updateCenterAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 */

/**
 * @member server.paraview.web.ParaViewWebViewPort
 * @method updateCamera
 * @param {String} viewId
 * @param {Array} focalPoint
 * @param {Array} viewUp
 * @param {Array} position
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebViewPortImageDelivery
 *
 * This protocol handle image delivery
 */

/**
 * @member server.paraview.web.ParaViewWebViewPortImageDelivery
 * @method stillRender
 * @param {Object} option
 *
 *    option = {
 *         size: [200, 300],  // [Optional] if not provided will use server side size. Size of the Viewport for which the image should be render for.
 *         view: -1,          // -1 for active one otherwise viewId
 *         quality: 100,      // JPEG compression level (100: best quality but big / 1: crappy but small)
 *         mtime: 2345233     // [Optional] Last MTime seen for that given view. If same as server, the reply will not provide the image payload.
 *         localtime: 3563456 // Local time at sending for round trip computation statistic
 *    }
 *
 * @return {Object} imageResult
 *
 *     imageResult = {
 *         image: "...",        // Base64 of the image
 *         stale: false,        // Image is stale
 *         mtime: 2345234,      // Timestamp for the given image
 *         size: [200,300],     // True size of the provided image
 *         format: "jpeg;base64",
 *         global_id: 2345,     // ViewId where that image was generated from.
 *         localTime: 3563456,  // Server system time. (Used for statistics)
 *         workTime: 14,        // Server processing time in ms. (Used for statistics)
 *     }

 * __stale__:
 * For better frame-rates when interacting, vtkWeb may
 * return a stale rendered image, while the newly rendered
 * image is being processed. This flag indicates that a new
 * rendering for this view is currently being processed on
 * the server.
 *
 * __mtime__:
 * Timestamp of the generated image. This is used to prevent
 * a redelivery of the same image.
 *
 * __localTime__:
 * Unchanged value that was in the request. This will help
 * to compute round trip cost.
 *
 * __workTime__:
 * Delta time that was needed on the server side to handle
 * the request. This does not include the json parsing.
 * Just the high level opeartion achieved by vtkWeb.
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebViewPortGeometryDelivery
 *
 * This protocol handle geometry delivery for local rendering using WebGL
 */

/**
 * @member server.paraview.web.ParaViewWebViewPortGeometryDelivery
 * @method getSceneMetaData
 * @param {String} viewId
 * @return {Object} metadata
 *
 *     metadata = {
 *         scene description...
 *     }
 */

/**
 * @member server.paraview.web.ParaViewWebViewPortGeometryDelivery
 * @method getWebGLData
 * @param {String} viewId
 * @param {String} objectId
 * @param {Number} part
 * @return {Object} binaryWebGLData
 *
 *     binaryWebGLData = {
 *         ...
 *     }
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebTimeHandler
 *
 * This protocol handle Time for time dependant Dataset
 */

/**
 * @member server.paraview.web.ParaViewWebTimeHandler
 * @method updateTime
 * @param {String} action
 * Action can be ['next', 'prev', 'first', 'last']
 * @return {Number} time
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebPipelineManager
 *
 * This protocol handle PipelineBrowser interactions
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method reloadPipeline
 * @return {Object} rootNode
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method getPipeline
 * @return {Object} rootNode
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method addSource
 * @param {String} algo_name
 * @param {String} parentId
 * @return {Object} proxyInfo
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method deleteSource
 * @param {String} proxyId
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method updateDisplayProperty
 * @param {Object} options
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method pushState
 * @param {Object} state
 * @return {Object} proxyInfo
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method openFile
 * @param {String} path
 * @return {Number} proxyInfo
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method openRelativeFile
 * @param {String} relativePath
 * @return {Number} proxyInfo
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method updateScalarbarVisibility
 * @param {Object} options
 * @return {Object} visibilityInfo
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method updateScalarRange
 * @param {String} proxyId
 */

/**
 * @member server.paraview.web.ParaViewWebPipelineManager
 * @method listFilters
 * @return {Object} listOfFilters
 *
 *     [{
 *         'name': 'Cone',
 *         'icon': 'dataset',
 *         'category': 'source'
 *     },{
 *         'name': 'Sphere',
 *         'icon': 'dataset',
 *         'category': 'source'
 *     },{
 *         'name': 'Wavelet',
 *         'icon': 'dataset',
 *         'category': 'source'
 *     },{
 *         'name': 'Clip',
 *         'icon': 'clip',
 *         'category': 'filter'
 *     },{
 *         'name': 'Slice',
 *         'icon': 'slice',
 *         'category': 'filter'
 *     },{
 *         'name': 'Contour',
 *         'icon': 'contour',
 *         'category': 'filter'
 *     },{
 *         'name': 'Threshold',
 *         'icon': 'threshold',
 *         'category': 'filter'
 *     },{
 *         'name': 'StreamTracer',
 *         'icon': 'stream',
 *         'category': 'filter'
 *     },{
 *         'name': 'WarpByScalar',
 *         'icon': 'filter',
 *         'category': 'filter'
 *     }]
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebFileManager
 *
 * DEPRECATED
 * This protocol handle file listing.
 */

/**
 * @member server.paraview.web.ParaViewWebFileManager
 * @method listFiles
 * @return {Object} listOfFiles
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebRemoteConnection
 *
 * This protocol handle remote connection to pvserver
 */

/**
 * @member server.paraview.web.ParaViewWebRemoteConnection
 * @method connect
 * @param {Object} options
 *
 *     options = {
 *          'host': 'localhost',
 *          'port': 11111,
 *          'rs_host': None,
 *          'rs_port': 11111
 *     }
 */

/**
 * @member server.paraview.web.ParaViewWebRemoteConnection
 * @method reverseConnect
 * @param {Number} port
 */

/**
 * @member server.paraview.web.ParaViewWebRemoteConnection
 * @method pvDisconnect
 */

 // =====================================================================

/**
 * @class server.paraview.web.ParaViewWebStateLoader
 *
 * This protocol handle state file loading at startup
 */

/**
 * @member server.paraview.web.ParaViewWebStateLoader
 * @method loadState
 * @param {String} state_file
 */

 // =====================================================================

 /**
 * @class server.paraview.web.ParaViewWebFileListing
 *
 * This protocol handle file listing for vtkWeb file browser widget.
 */

/**
 * @member server.paraview.web.ParaViewWebFileListing
 * @method listServerDirectory
 * @param {String} relativeDir
 * @return {Object} listOfFiles
 *
 *     {
 *       label: 'relativeDir',
 *       files: [ {label: 'fileName'}],
 *       dirs: ['a','b','c'],
 *       groups: [ { files: ['file_001.vtk','file_002.vtk','file_003.vtk'], label: 'file_*.vtk'}],
 *       path: [ 'Home', 'parentDir']
 *     }
 */

 // =====================================================================

/**
 * jQuery JavaScript Library.
 *
 * @class jQuery
 *
 * @mixins jQuery.paraview.ui.PipelineBrowser
 * @mixins jQuery.paraview.ui.toolbar.vcr
 * @mixins jQuery.paraview.ui.toolbar.viewport
 *
 * @singleton
 */
