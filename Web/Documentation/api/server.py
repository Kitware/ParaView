/**
 * @class server.WebServer
 * This Python class will start a webserver on a given port and will
 * serve static content available in the "content" directory as well
 * as extend a WAMP session with a given protocol.
 * 
 * @property port
 * Port number
 * 
 * @property content
 * Path to the static content
 * 
 * @property protocol
 * Name of the protocol that should be used by WAMP
 * 
 *     pvpython default.py --port=8080 --content=/.../web-content --protocol=ParaViewProtocol
 */
