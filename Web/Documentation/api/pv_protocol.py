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
 *      session.call("pv.service.name.method", [arg1, arg2]).then(function(response) {
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
 * Registered as viewport.mouse.interaction
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
 *
 * Registered as viewport.camera.reset
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateOrientationAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 *
 * Registered as viewport.axes.orientation.visibility.update
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateCenterAxesVisibility
 * @param {String} viewId
 * @param {boolean} show
 *
 * Registered as viewport.axes.center.visibility.update
 */

/**
 * @member protocols.ParaViewWebViewPort
 * @method updateCamera
 * @param {String} viewId
 * @param {Array} focalPoint
 * @param {Array} viewUp
 * @param {Array} position
 *
 * Registered as viewport.camera.update
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
 * Registered as viewport.image.render
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
 * Registered as viewport.webgl.metadata
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
 * Registered as viewport.webgl.data
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
 *
 * Registered as pv.vcr.action
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebColorManager
 * @new
 *
 * This protocol handles interactions related to color management on
 * pipeline nodes.
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method __init__
 *
 * The constructor allows to specify the location of a file containing
 * the color map presets that should be used.  This is an xml file with the
 * following format:
 *
 *     <ColorMaps>
 *         <ColorMap space="HSV" indexedLookup="false" name="Blue to Red Rainbow">
 *             <Point x="0" o="0" r="0" g="0" b="1"/>
 *             <Point x="1" o="0" r="1" g="0" b="0"/>
 *             <NaN r="0.498039" g="0.498039" b="0.498039"/>
 *         </ColorMap>
 *         ...
 *     </ColorMaps>
 *
 * @param {String} pathToColorMaps
 *
 * A string containing the path to the desired color maps file.  If none
 * is specified, then the set of color maps typically used in ParaView
 * is provided by default.
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method rescaleTransferFunction
 *
 * Registered as pv.color.manager.rescale.transfer.function
 *
 * Rescale the color transfer function to fit either the data range,
 * the data range over time, or to a custom range, for the array by
 * which the representation is currently being colored.
 *
 * @param {Object} options
 *
 * An object containing the desired options for the transfer function
 * rescale.  Possibilities are to rescale to the entire time series,
 * just to the data, or to a custom range.  If the 'extend' option is
 * true, then the range will only be extended, and then only if needed.
 * The default for the 'extend' option is false, which will allow
 * rescaling of the range that results in a contraction of the currently
 * set color range.
 *
 *     options = {
 *         'type': 'time' | 'data' | 'custom',  // string, required
 *         'proxyId': 352,                      // integer, required
 *         'extend': true | false,              // boolean, optional, default is false
 *         'min': 10,                           // float, required for 'custom'
 *         'max': 90                            // float, required for 'custom'
 *     }
 *
 * The keys 'type' and 'proxyId' are required.  The key 'extend' is
 * optional, but is only used if 'type' is 'data' or 'custom', and
 * is ignored 'type' is time.  If not provided, 'extend' defaults
 * to false.  The keys 'min' and 'max' are required if 'type' is
 * 'custom', and it is an error if they are not provided.
 *
 * @return {Object} status
 *
 * Returns an object which contains at least the key 'success' mapping
 * to either true or false.
 *
 *     status = {
 *         'success': true | false
 *     }
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method getScalarBarVisibilities
 *
 * Registered as pv.color.manager.scalarbar.visibility.get
 *
 * Returns whether or not each specified scalar bar is visible
 *
 * @param {Object} proxies
 *
 * This parameter can actually be either a list:
 *
 *     [ '253', '457', '631' ]
 *
 * or a dictionary, where the values will be ignored:
 *
 *     {
 *         '253': '',
 *         '457': '',
 *         '631': ''
 *     }
 *
 * @return {Object} visibilities
 *
 * Returns a dictionary mapping each proxyId specified in the parameter
 * to a boolean value indicating whether the scalar bar is visible for
 * the representation given by the proxy id and the current view.  This
 * object will have the form of the dictionary parameter documented for
 * this method.
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method setScalarBarVisibilities
 *
 * Registered as pv.color.manager.scalarbar.visibility.set
 *
 * Sets the visibility of the scalar bar corresponding to each specified
 * proxy.  The representation for each proxy is found using the
 * filter/source proxy id and the current view.
 *
 * Note that all proxies colored by the same array name share a color
 * transfer function (lookup table), and therefore also share a scalar
 * bar representation, so the behavior is undefined if you specify that
 * multiple proxy ids should have different scalar bar visibilities when
 * they are all being colored by the same array name.  Additionally, if
 * you turn on scalar bar visibility for one proxy, then all proxies
 * being colored by the same array name will appear to have their
 * scalar bars turned on as well.
 *
 * @param {Object} visibilities
 *
 * A map containing the proxy ids as keys and the desired visibilities of the
 * corresponding scalar bars as values.
 *
 *     visibilities = {
 *         '253': true,
 *         '457': false,
 *         '631': true
 *     }
 *
 * @return {Object} visibilities
 *
 * Returns an object of the same form as the parameter, where the boolean
 * values are determined by the actual visibilities of the scalar bars
 * for each proxy, as determined after the attempted set operation.
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method colorBy
 *
 * Registered as pv.color.manager.color.by
 *
 * Choose the array to color by, and optionally specify magnitude or a
 * vector component in the case of vector array.
 *
 * @param {String} representation
 *
 * Required.  The id of the representation on which to change the color.
 *
 * @param {String} colorMode
 *
 * Required.  Must be either 'SOLID' or 'ARRAY'.
 *
 * @param {String} [arrayLocation="POINTS"]
 *
 * Must be one of 'POINTS' or 'CELLS'.
 *
 * @param {String} [arrayName=""]
 *
 * The name of the array you wish to color by.
 *
 * @param {String} [vectorMode="Magnitude"]
 *
 * This parameter should be 'Magnitude' if you wish to color by the
 * vector magnitude, or 'Component' if you wish to color by one of the
 * vector components. If the array does not contain vector values, but
 * rather scalars, then 'Magnitude' will color by the scalar value.
 *
 * @param {Number} [vectorComponent=0]
 *
 * If you wish to color by a specific component of a vector-valued array,
 * use this parameter to specify the index of the component.
 *
 * @param {Boolean} [rescale=False]
 *
 * If True, the fuction will rescale the transfer function to the data
 * range after selecting the array to color by.  If False, no rescale
 * will be done.
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method selectColorMap
 *
 * Registered as pv.color.manager.select.preset
 *
 * Choose the color map preset to use when coloring by an array.
 *
 * @param {String} representation
 *
 * The id of the representation to which you wish to apply the color map.
 *
 * @param {String} paletteName
 *
 * The name of the color map preset you wish to apply.  This name should
 * be chosen from the list of available color map preset which you can
 * query by calling the #listColorMapNames function.
 *
 * @return {Object}
 *
 * An object containing the results, at least the 'result' key will
 * be present, but also more information may be provided.
 *
 *     {
 *         'result': 'success' | 'preset nameOfPresetColorMap not found'
 *     }
 */

/**
 * @member protocols.ParaViewWebColorManager
 * @method listColorMapNames
 *
 * Registered as pv.color.manager.list.preset
 *
 * List the names of all color map presets available on the server.  This
 * list will contain the names of any presets you provided in the file
 * you supplied to the constructor of this protocol.
 *
 * @return {String[]}
 *
 * A list of strings naming the individual color map presets that are
 * available.  These are the names you can successfuly use when you invoke
 * the rpc method 'selectColorMap'.
 *
 *     [
 *         "Preset Name #1",
 *         "Preset Name #2",
 *          ...
 *     ]
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebProxyManager
 * @new
 *
 * This protocol allows developers to perform CRUD operations on ParaView
 * proxies.
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method __init__
 *
 * Construct an instance of the proxy manager class.
 *
 * @param {String} [allowedProxiesFile=None]
 *
 * A string containing the path to a file containing a json object giving
 * the kinds of sources, filters, and readers that can be created by the
 * proxy manager.  If no such file is specified, a default set of
 * capabilities will be available  The file takes the following format:
 *
 *     {
 *         "sources": [
 *
 *             { "name": "AnnotateTime",
 *               "label": "Annotate Time" },
 *
 *             { "name": "Box" },
 *
 *             { "name": "Wavelet" },
 *
 *             ...
 *         ],
 *
 *         "filters": [
 *
 *             { "name": "Calculator" },
 *
 *             { "name": "CellDataToPointData",
 *               "label": "Cell Data To Point Data" },
 *
 *             { "name": "Clip" },
 *
 *             ...
 *         ],
 *
 *         "readers": [
 *             { "name": "LegacyVTKReader",
 *               "extensions": [ "vtk" ],
 *               "method": "FileNames" }
 *         ]
 *     }
 *
 * Some details about the "readers" section:  The "name" attribute should be
 * a name which will be recognized by the paraview.simple module.  If any file
 * to be opened has an extension found in the "extensions" list, then this
 * reader will be used to open it.  Some readers have a FileName property,
 * others have a FileNames property, and these are used to set the
 * filenames to be read.  If no "method" is provided in a reader
 * configuration section, then it will be assumed that FileName should be
 * used.
 *
 * @param {String} [baseDir=None]
 *
 * The root directory where the proxy manager should look for data files
 * when the #open method is called.
 *
 * @param {Boolean} [allowUnconfiguredReaders=False]
 *
 * When attempting to open a file, the configured proxies file will be
 * used to try and map a file extension to the appropriate reader.  If
 * no such mapping is found for a file, this variable determines whether
 * or not to use the default file opening mechanism found in the
 * paraview.simple module.
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method get
 *
 * Registered as pv.proxy.manager.get
 *
 * Get all the information about a proxy given its global id.
 *
 * @param {String} proxyId
 *
 * The id of the proxy for which you want the details
 *
 * @return {Object}
 *
 * Returns a JSON object containing several sections: The 'properties' and 'ui'
 * sections are parallel lists describing all the proxy properties,
 * including information about how to render them.  The ordering of these
 * lists corresponds to the property order as given by the proxy property
 * xml files.
 *
 * If the proxyId belongs to a source or filter proxy, then a 'data'
 * section will be included in the result, whereas if the proxyId belongs
 * to a representation proxy, then a 'colorBy' sections will be included.
 * Also possibly included might be a 'hints' section.  The following
 * listing illustrates the kind of information returned by this function:
 *
 *     {
 *
 *         'id': '445',
 *
 *         'properties': [       ### 'properties' and 'ui' are parallel arrays
 *             {
 *                 'id': '635',
 *                 'name': 'ClipFunction',
 *                 'value': 'Plane'
 *             }, ...
 *         ],
 *
 *         'ui': [               ### 'properties' and 'ui' are parallel arrays
 *             {
 *                 'name': 'Clip Type',
 *                 'advanced': 1,         # 0
 *                 'depends': '498:ClipFunction:Plane:[1|0]',
 *                 'doc': 'Documentation for property from xml description',
 *                 'values': { 'Plane': '456', 'Box': '457',
 *                             'Scalar': '458', 'Sphere': '459' },
 *                 'type': 'int',            # 'float', 'int', 'str', 'proxy', 'input'
 *                 'widget': 'textfield',    # 'checkbox', 'textarea', 'list-1', 'list-n'
 *                 'size': -1,               # -1, 0, 2, 3, 6
 *                 'range': [ { 'min': 0, 'max': 1 }, { 'min': 4, 'max': 7 }, ... ]
 *                 # 'range' will contain a single element if 'size' is -1 or 0, and
 *                 # will contain 'size' elements otherwise.
 *             }, ...
 *         ],
 *
 *         'data': {             ### Only present when proxyId is a source proxy id
 *             'points': 765239,
 *             'cells': 1272,
 *             'type': 'Multi-Block Dataset',
 *             'time': ['0', '1', '2', ... ],
 *             'memory': 2171,    # in Kilobytes
 *             'bounds': [ -10, 10, -10, 10, -5, 5 ],
 *             'arrays': [
 *                 {
 *                     'location': 'POINTS',  # 'CELLS'
 *                     'type': 'double',      # 'idtype', 'int', 'char', 'string'
 *                     'name': 'DISPL',
 *                     'size': 3,
 *                     'range': [
 *                         # if 'size' is 1, there will be only one range element
 *                         # and it will have an empty 'name'.
 *                         { 'min': 0, 'max': 219, 'name': 'Magnitude' },
 *                         { 'min': 1, 'max': 100, 'name': 'X' },
 *                         { 'min': 0, 'max': 50, 'name': 'Y' },
 *                         { 'min': 7, 'max': 21, 'name': 'Z' }
 *                     ]
 *                 }, ...
 *             ]
 *         },
 *
 *         'hints': {            ### Only if proxy has 'Hints' xml with 'replace_input'
 *             'replaceInput': 2   # 0, 1, 2, ...
 *         },
 *
 *         'colorBy': {          ### Only present when proxyId is a repr proxy id
 *             'representation': '357',
 *             'scalarBar': 0,                        # 0, 1
 *             'mode': 'color'                        # 'array'
 *             'color': [ 0.5, 1.0, 1.0 ],
 *             'array': [ 'POINTS', 'RTData', -1 ]    # only present if 'mode' is 'array'
 *         }
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method create
 *
 * Registered as pv.proxy.manager.create
 *
 * Create a new proxy whose Input property will be the proxy given
 * by the supplied parent proxy id.  In order to create a proxy, it
 * must be specified in the allowed proxies configuration, see the
 * constructor documentation for details on the structure of the
 * proxies configuration file.
 *
 * @param {String} functionName
 *
 * The name (or label, if label existed in the proxy configuration)
 * of the proxy to be created.
 *
 * @param {String} parentId
 *
 * The id of the proxy which should act as the input to the new proxy.
 *
 * @return {Object}
 *
 * Returns the JSON object that results from calling the #get method
 * on the newly created proxy.  In the case that the proxy could not
 * be created, a JSON object indicating failure and the reason why
 * should be returned:
 *
 *     {
 *         'success': False,
 *         'reason': 'Wav3let was not valid and could not be created'
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method open
 *
 * Registered as pv.proxy.manager.create.reader
 *
 * Open a file relative to the configured base directory.  Behavior
 * when opening files is governed by the configured proxies file, as
 * well as the allowUnconfiguredReaders parameter to the constructor.
 * See the constructor documentation for more information.
 *
 * @param {String} relativePath
 *
 * The relative path to the file.
 *
 * @return {Object}
 *
 * Returns a JSON object.  In the case of a successful open, the id of
 * new reader proxy is returned:
 *
 *     {
 *         'id': '561'
 *     }
 *
 * In case of failure:
 *
 *     {
 *         'success': False,
 *         'reason': 'No configured reader found for vtk files, and unconfigured readers are not enabled.'
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method update
 *
 * Registered as pv.proxy.manager.update
 *
 * Update a list of properties.
 *
 * @param {Object[]} propertiesList
 *
 * A list of the properties you wish to update, which could be on
 * the same or different proxies.  An simple example:
 *
 *     [
 *         {
 *             'id': '321',
 *             'name': 'Radius',
 *             'value': 3.0
 *         },
 *         {
 *             'id': '416',
 *             'name': 'ClipFunction',
 *             'value': 'Sphere'
 *         }, ...
 *     ]
 *
 * @return {Object}
 *
 * Returns a JSON object indicating either success or failure.  In
 * the case of failure, a list of reasons is included in the result,
 * indicating a reason for any failed property updates.  For example:
 *
 *     {
 *         'success': False,
 *         'errorList': [ 'Unable to set property for proxy 317: Representations' ]
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method delete
 *
 * Registered as pv.proxy.manager.delete
 *
 * Delete a proxy, as long as it is not currently the input to any
 * other existing proxy.
 *
 * @param {String} proxyId
 *
 * The proxy id of the proxy to be deleted.
 *
 * @return {Object}
 *
 * Returns a JSON object indicating success or failure as well as the id
 * of the deleted proxies parent (or else '0').  A success example:
 *
 *     {
 *         'success': 1,
 *         'id': '365'
 *     }
 *
 * In case of failure:
 *
 *     {
 *         'success': 0,
 *         'id': '0'
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method list
 *
 * Registered as pv.proxy.manager.list
 *
 * Return a list of all sources and filters associated to a view.
 *
 * @param {String} [viewId=None]
 *
 * If no view proxy id string is provided, the current active
 * view will be assumed.
 *
 * @return {Object}
 *
 * Returns a JSON object describing the state of the pipeline, and
 * including the id of the view used.  Each proxy element in the
 * returned list will indicate its parent proxy, and in the case
 * multiple input proxies, all of the input proxies are referred
 * to as in the last proxy in the example below:
 *
 *     {
 *         'view': '370',
 *         'sources': [
 *             {
 *                 'name': 'Wavelet0', 'id': '350',  'parent': '0', 'visible': 0, 'rep': '281'
 *             },
 *             {
 *                 'name': 'Clip0', 'id': '455', 'parent': '350', 'visible': 1, 'rep': '467'
 *             },
 *             {
 *                 'name': 'PF1', 'id': '276', 'parent': '455', 'visible': 1, 'rep': '541'
 *             },
 *             {
 *                 'name': 'ProgrammableFilter0', 'id': '641',  'multiparent': 2, 'parent': '455', 'parent_1': '276', 'visible': 0, 'rep': '716'
 *             }, ...
 *         ]
 *     }
 */

/**
 * @member protocols.ParaViewWebProxyManager
 * @method available
 *
 * Registered as pv.proxy.manager.available
 *
 * Returns a list of the sources or filters which are available to be
 * created.  See the constructor documentation for notes on how to
 * add to the default list of sources and filters.
 *
 * @param {String} typeOfProxy
 *
 * Request either the sources or the filters list.  This parameter must
 * be one of 'sources' or 'filters'.
 *
 * @return {String[]}
 *
 * Returns a list of the names that can be used to create sources or
 * filters, for example:
 *
 *     [ 'Wavelet', 'Cone', 'Sphere', ... ]
 *
 * or
 *
 *     [ 'Contour', 'Clip', 'Slice', ... ]
 */

 // =====================================================================

/**
 * @class protocols.ParaViewWebPipelineManager
 * @deprecated Use {@link protocols.ParaViewWebProxyManager} and
 * {@link protocols.ParaViewWebColorManager} instead.
 *
 * This protocol handles PipelineBrowser interactions.
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method reloadPipeline
 * @return {Object} rootNode
 *
 * Registered as pv.pipeline.manager.reload
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method getPipeline
 * @return {Object} rootNode
 *
 * Registered as pv.pipeline.manager.pipeline.get
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method addSource
 * @param {String} algo_name
 * @param {String} parentId
 * @return {Object} proxyInfo
 *
 * Registered as pv.pipeline.manager.proxy.add
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method deleteSource
 * @param {String} proxyId
 *
 * Registered as pv.pipeline.manager.proxy.delete
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateDisplayProperty
 * @param {Object} options
 *
 * Registered as pv.pipeline.manager.proxy.representation.update
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method pushState
 * @param {Object} state
 * @return {Object} proxyInfo
 *
 * Registered as pv.pipeline.manager.proxy.update
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method openFile
 * @param {String} path
 * @return {Number} proxyInfo
 *
 * Registered as pv.pipeline.manager.file.open
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method openRelativeFile
 * @param {String} relativePath
 * @return {Number} proxyInfo
 *
 * Registered as pv.pipeline.manager.file.ropen
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateScalarbarVisibility
 * @param {Object} options
 * @return {Object} visibilityInfo
 *
 * Registered as pv.pipeline.manager.scalarbar.visibility.update
 */

/**
 * @member protocols.ParaViewWebPipelineManager
 * @method updateScalarRange
 * @param {String} proxyId
 *
 * Registered as pv.pipeline.manager.scalar.range.rescale
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
 *
 * Registered as pv.pipeline.manager.lut.range.get
 *
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
 * Registered as file.server.directory.list
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
 * This protocol handle file listing.
 *
 * @deprecated
 * Should be replaced by protocols.ParaViewWebFileListing
 */

/**
 * @member protocols.ParaViewWebFileManager
 * @method listFiles
 * @return {Object} listOfFiles
 *
 * Registered as pv.files.list
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
 * Registered as pv.remote.connect
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
 *
 * Registered as pv.remote.reverse.connect
 */

/**
 * @member protocols.ParaViewWebRemoteConnection
 * @method pvDisconnect
 *
 * Registered as pv.remote.disconnect
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
 *
 * Registered as pv.loader.state
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
 * Registered as pv.server.directory.list
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
