default: jsSocket.swf jsSocket2.swf

jsSocket.swf: jsSocket.as
	mtasc jsSocket.as -version 8 -main -mx -header 800:600:31 -swf jsSocket.swf

jsSocket2.swf: JsSocket.hx
	haxe -swf jsSocket2.swf -swf-version 9 -main JsSocket -swf-header 800:600:31
