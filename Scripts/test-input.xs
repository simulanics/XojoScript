var userInput as String 

Print("Enter some text, press 'Enter.' Input 'exit()' to quit")

While userInput <> "exit()"
	userInput= Input()
		If userInput = "exit()" then 
			return 0
		End If
	var y() as String = Split(userInput, "")
	var t as String
	For i as integer = y.LastIndex() DownTo 0
		t = t + y(i)
	Next
	
	Print("Your text backwards: " + t)
Wend