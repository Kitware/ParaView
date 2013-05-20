/**
 * @class server.WebServer
 * This Python class will start a webserver on a given port and will
 * serve static content available in the "content" directory as well
 * as extend a WAMP session with a given protocol.
 *
 * @property port
 * Port number on which the HTTP server will listen to.
 *
 * @property content
 *  Directory that you want to server as static web content.
 *  By default, this variable is empty which mean that we rely on another server
 *  to deliver the static content and the current process only focus on the
 *  WebSocket connectivity of clients.
 *
 * @property authKey
 * Secret key that should be provided by the client to allow it to make any
 * WebSocket communication. The client will assume if none is given that the
 * server expect "paraviewweb-secret" as secret key.
 *
 * @property data-dir
 * Used to list that directory on the server and let the client choose a file to load.
 * (Property specific to pipeline_manager.py and file_loader.py)
 *
 *     $ pvpython .../pipeline_manager.py --data-dir /.../path-to-your-data-directory
 */
