// This file is used to generate online documentation for ParaView protocols.

/**
 * @class protocols.ParaViewWebProtocol
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
 * @class protocols.ParaViewWebMouseHandler
 *
 * This protocol handle mouse interaction requests.
 */

/**
 * @member protocols.ParaViewWebMouseHandler
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
 * @class protocols.ParaViewWebViewPort
 *
 * This protocol handle view common methods.
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method resetCamera
 * @param {String} viewId
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateOrientationAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateCenterAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateCamera
 * @param {String} viewId
 * @param {Array} focalPoint
 * @param {Array} viewUp
 * @param {Array} position
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebViewPortImageDelivery
 *
 * This protocol handle image delivery
 */

/**
 * @member protocols.ParaViewWebViewPortImageDelivery
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
 * @class protocols.ParaViewWebViewPortGeometryDelivery
 *
 * This protocol handle geometry delivery for local rendering using WebGL
 */

/**
 * @member protocols.ParaViewWebViewPortGeometryDelivery
 * @method getSceneMetaData
 * @param {String} viewId
 * @return {Object} metadata
 *
 *     metadata = {
 *         scene description...
 *     }
 */

/**
 * @member protocols.ParaViewWebViewPortGeometryDelivery
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
 * @class protocols.ParaViewWebTimeHandler
 *
 * This protocol handle Time for time dependant Dataset
 */

/**
 * @member protocols.ParaViewWebTimeHandler
 * @method updateTime
 * @param {String} action
 * Action can be ['next', 'prev', 'first', 'last']
 * @return {Number} time
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebPipelineManager
 *
 * This protocol handle PipelineBrowser interactions
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method reloadPipeline
 * @return {Object} rootNode
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method getPipeline
 * @return {Object} rootNode
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method addSource
 * @param {String} algo_name
 * @param {String} parentId
 * @return {Object} proxyInfo
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method deleteSource
 * @param {String} proxyId
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateDisplayProperty
 * @param {Object} options
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method pushState
 * @param {Object} state
 * @return {Object} proxyInfo
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method openFile
 * @param {String} path
 * @return {Number} proxyInfo
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method openRelativeFile
 * @param {String} relativePath
 * @return {Number} proxyInfo
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateScalarbarVisibility
 * @param {Object} options
 * @return {Object} visibilityInfo
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateScalarRange
 * @param {String} proxyId
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method setLutDataRange
 *
 *     Set a new data range for the specified array.  Compression and
 * expansion is allowed, and the new data range will be mapped into the
 * the full range of colors.
 *
 * @param {String} name
 *
 *     The name of the data array for which change is to be applied
 *
 * @param {Number} number_of_components
 *
 *     The number of components per element for this data array
 *
 * @param {Number[]} customRange
 *
 *     The new data range ([min, max]) to map into the full range of colors
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method getLutDataRange
 * @param {String} name
 *
 *     The name of the data array for which data range is to be returned
 *
 * @param {Number} number_of_components
 *
 *     The number of components per element for this data array
 *
 * @return {Number[]} currentDataRange
 *
 * An array containing the currently set data range ([min, max]) for the
 * specified data array.
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebFilterList
 *
 * This protocol handles listing supported filters, and can be configured
 * with the file path to a filter list file containing json text giving
 * the filters which should be supported.  By default, however, a static
 * set of filters will be provided.
 */

/**
 * @member protocols.ParaViewWebFilterList
 * @method listFilters
 * @return {Object} listOfFilters
 *
 * If a file path is given to the constructor of this protocol, then the
 * available filters will be those defined in the file.  Otherwise, the
 * following static list will be available:
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
 * @class protocols.ParaViewWebFileManager
 *
 * DEPRECATED
 * This protocol handle file listing.
 */

/**
 * @member protocols.ParaViewWebFileManager
 * @method listFiles
 * @return {Object} listOfFiles
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebRemoteConnection
 *
 * This protocol handle remote connection to pvserver
 */

/**
 * @member protocols.ParaViewWebRemoteConnection
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
 * @member protocols.ParaViewWebRemoteConnection
 * @method reverseConnect
 * @param {Number} port
 */

/**
 * @member protocols.ParaViewWebRemoteConnection
 * @method pvDisconnect
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebStartupRemoteConnection
 *
 * This protocol handles a remote connection at startup.
 */

/**
 * @member protocols.ParaViewWebStartupRemoteConnection
 * @method __init__
 *
 *     The constructor takes four parameters and allows configuration
 * of host names and port numbers for a data server or render server.
 *
 * @param {String} dsHost
 *
 *     The host name of the data server, defaults to None.
 *
 * @param {Number} dsPort
 *
 *     The port number of the data server, defaults to 11111.
 *
 * @param {String} rsHost
 *
 *     The host name of the render server, defaults to None.
 *
 * @param {Number} rsPort
 *
 *     The port number of the render server, defaults to 22222.
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebStartupPluginLoader
 *
 * This protocol handles loading a list of plugins at startup.
 */

/**
 * @member protocols.ParaViewWebStartupPluginLoader
 * @method __init__
 *
 *     The constructor allows parameters which control which plugins
 * should be loaded by the application upon startup.
 *
 * @param {String} plugins
 *
 *     A string containing the paths to library files representing plugins
 * to load.  The paths should be separated by ':' unless a different
 * separator is given by the next parameter.
 *
 * @param {String} pathSeparator
 *
 *     A string containing the path separator for the previous parameter.
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebStateLoader
 *
 * This protocol handle state file loading at startup
 */

/**
 * @member protocols.ParaViewWebStateLoader
 * @method loadState
 * @param {String} state_file
 */

 // =====================================================================

 /**
 * @class protocols.ParaViewWebFileListing
 *
 * This protocol handle file listing for vtkWeb file browser widget.
 */

/**
 * @member protocols.ParaViewWebFileListing
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
