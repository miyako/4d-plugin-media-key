$event:=Form event code:C388

Case of 
	: ($event=On Load:K2:1)
		
		C_OBJECT:C1216($context)
		$context:=New object:C1471("window"; Current form window:C827; "processName"; Current process name:C1392; "method"; "EVENT")
		
		ON MEDIA KEY CALL("TEST"; JSON Stringify:C1217($context); KEYEVENT_LOCAL)
		
	: ($event=On Unload:K2:2)
		
		ON MEDIA KEY CALL("")
		
End case 