/**
 * ParaViewWeb JavaScript Library.
 *
 * This module allow the Web client to start a remote ParaViewWeb session and
 * retreive all the connection informations needed to properly connect to that
 * newly created session.
 *
 * @class paraview.launcher
 *
 * {@img paraview/ParaViewWeb-multiuser.png alt Focus on the communication between the client and the front-end that manage the ParaView processes}
 */
(function (GLOBAL, $) {

    // Internal field used to store all connection objects
    var Connections = [], module = {}, console = GLOBAL.console;

    /**
     * @class pv.ConnectionConfig
     * This class provides all the informations needed to connect to the session
     * manager web service.
     */
    /**
     * @member pv.ConnectionConfig
     * @property {String} url
     * The service URL that will respond to the REST request to start or stop
     * a visualization session.
     *
     * MANDATORY
     */
    /**
     * @member pv.ConnectionConfig
     * @property {String} name
     * The name given for the visualization.
     *
     * RECOMMENDED/OPTIONAL
     */
    /**
     * @member pv.ConnectionConfig
     * @property {String} application
     * The name of the application that should be started on the server side.
     *
     * MANDATORY
     */
    /**
     * @member pv.ConnectionConfig
     * @property {String|Number} __Any_Name__
     * Any property that we want to provide to the session that will be created.
     * Such property is not necessary used by the session manager but will be
     * returned if a connection information is requested from a session.
     *
     * OPTIONAL
     */

    //=========================================================================

    /**
     * @class pv.Connection
     * This class provides all the informations needed to connect to a running
     * visualization session.
     *
     * @mixins pv.ConnectionConfig
     */
    /**
     * @member pv.Connection
     * @property {String} wampURL
     * The websocket URL that should be used to connect to the running
     * visualization session.
     */
    /**
     * @member pv.Connection
     * @property {String} id
     * The session identifier.
     */
    /**
     * @member pv.Connection
     * @property {pv.Session} session
     * The session object will be automatically added to the connection once the
     * connection is properly established by calling:
     *
     *     paraview.connect(connection, success, error);
     */
    //=========================================================================

    /**
     * Start a new ParaView process on the server side.
     * This method will make a JSON POST request to config.url URL.
     *
     * @member paraview.launcher
     *
     * @param {pv.ConnectionConfig} config
     * Session creation parameters. (url, name, application).
     *
     * @param {Function} successCallback
     * The function will be called once the connection is successfully performed.
     * The argument of the callback will be a {@link pv.Connection}.
     *
     * @param {Function} errorCallback
     * The function will be called if anything bad happened and an explanation
     * message will be provided as argument.
     */
    function start(config, successFunction, errorFunction) {
        var okCallback = successFunction,
        koCallback = errorFunction,
        arg = {
            url: config.url,
            type: "POST",
            dataType: "json",
            data: (JSON.stringify(config)),
            success: function (reply) {
                Connections.push(reply);
                if (okCallback) {
                    okCallback(reply);
                }
            },
            error: function (errMsg) {
                if (koCallback) {
                    koCallback(errMsg);
                }
            }
        };
        return $.ajax(arg);
    }


    /**
     * Query the Session Manager in order to retreive connection informations
     * based on a session id.
     *
     * @member paraview.launcher
     *
     * @param {String} serviceUrl
     * Same as ConnectionConfig.url value.
     *
     * @param {String} sessionId
     * The unique identifier of a session.
     *
     * @return {pv.Connection} if the session is found.
     */
    function fetchConnection(serviceUrl, sessionId) {
        var config = {
            url: serviceUrl + '/' + sessionId,
            dataType: "json"
        };
        return $.ajax(config);
    }

    /**
     * Stop a remote running visualization session.
     *
     * @member paraview.launcher
     *
     * @param {pv.ConnectionConfig} connection
     */
    function stop(connection) {
        var config = {
            url: connection.url + "/" + connection.id,
            type: "DELETE",
            dataType: "json",
            success: function (reply) {
                console.log(reply);
            },
            error: function (errMsg) {
                console.log("Error while trying to close service");
            }
        };
        return $.ajax(config);
    }

    // ----------------------------------------------------------------------
    // Init ParaView module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("paraview")) {
        module = GLOBAL.paraview || {};
    } else {
        GLOBAL.paraview = module;
    }

    // ----------------------------------------------------------------------
    // Export internal methods to the ParaView module
    // ----------------------------------------------------------------------
    module.start = function (config, successFunction, errorFunction) {
        return start(config, successFunction, errorFunction);
    };
    module.stop = function (connection) {
        return stop(connection);
    };
    module.fetchConnection = function (serviceUrl, sessionId) {
        return fetchConnection(serviceUrl, sessionId);
    };
    /**
     * Return all the session connections created in that JavaScript context.
     * @member paraview.launcher
     * @return {pv.Connection[]}
     */
    module.getConnections = function () {
        return Connections;
    };

}(window, jQuery));
