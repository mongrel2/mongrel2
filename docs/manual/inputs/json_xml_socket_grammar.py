rel_path = ( path? (";" params)? ) ("?" query)?;
SocketJSONStart = ("@" rel_path);
SocketJSONData = "{" any* "}" :>> "\0";

SocketXMLData = ("<" [a-z0-9A-Z\-.]+)
    ("/" | space | ">") any* ">" :>> "\0";

SocketJSON = SocketJSONStart " " SocketJSONData;
SocketXML = SocketXMLData;

SocketRequest = (SocketXML | SocketJSON);
