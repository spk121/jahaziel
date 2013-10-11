jahaziel
========

A graphical client that renders a Videotex-like protocol

The client has three modes.

First it has a browse mode.  It can connect with brokers that use the Johanan/Jozabad protocol, get their list of available connections, register itself, and connect to a given available connection.

Second, it has a render mode.  It receives text information with embedded escape sequences in the Videotex protocol and renders them to a window.  It also sends keypresses and other data to the server.

Third, it has a built-in chat server mode.  It waits for another Jahaziel client to connect with it so that a peer-to-peer chat may occur.
