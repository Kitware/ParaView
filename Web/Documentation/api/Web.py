/**
 * @class server.ParaViewProtocol
 * Basic protocol used on the server side to handle client request.
 * This class provide the basic methods needed to perform interactive remote
 * rendering with a ParaView based engine.
 *
 * The given methods are exposed as RPC to the [Autobahn](http://autobahn.ws/js) session object.
 * Here is a RPC call example on how to make such a call.
 *
 *      session.call("pv:stillRender", options).then(function(paraviewEncodedImage) {
 *          // Can process the paraviewEncodedImage now
 *      });
 *
 */

/**
 * Render an image for a given viewport
 *
 * @param  {request.Render}    options
 * @return {reply.Render}
 */
function stillRender(options){}


/**
 * Trigger interaction event inside a given view.
 *
 * @param {request.InteractionEvent} event
 */
function mouseInteraction(event, view){}


/**
 * ResetCamera on the given view.
 *
 * @param {Number} viewId
 * The view proxy global id.
 */
function resetCamera(viewId){}


/**
 * Request the visualization process to exit.
 */
function exit(){}


/**
 * jQuery JavaScript Library.
 *
 * @class jQuery
 *
 * @mixins jQuery.paraview.PipelineBrowser
 * @singleton
 */
