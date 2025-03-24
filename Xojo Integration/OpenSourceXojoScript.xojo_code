#tag Module
Protected Module OpenSourceXojoScript
	#tag Method, Flags = &h0
		Function RunOSXojoscript(code as String, Optional enableDebug as Boolean = False) As String
		  // Declare the external function from the DLL.
		  #If TargetWindows Then
		    Soft Declare Function CompileAndRun Lib "xojoscript.dll" (code As CString, debugEnabled as Boolean) As CString
		  #ElseIf TargetMacOS Then
		    Soft Declare Function CompileAndRun Lib "libxojoscript.dylib" (code As CString, debugEnabled as Boolean) As CString
		  #Else
		    Soft Declare Function CompileAndRun Lib "libxojoscript.so" (code As CString, debugEnabled as Boolean) As CString
		  #EndIf
		  
		  // Call the external function.
		  
		  var c as CString = code
		  
		  Dim cResult As String = cstr(CompileAndRun(code, enableDebug))
		  
		  // Convert the C-string to a Xojo String and return it.
		  Return cResult
		  
		End Function
	#tag EndMethod


	#tag ViewBehavior
		#tag ViewProperty
			Name="Name"
			Visible=true
			Group="ID"
			InitialValue=""
			Type="String"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Index"
			Visible=true
			Group="ID"
			InitialValue="-2147483648"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Super"
			Visible=true
			Group="ID"
			InitialValue=""
			Type="String"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Left"
			Visible=true
			Group="Position"
			InitialValue="0"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Top"
			Visible=true
			Group="Position"
			InitialValue="0"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
	#tag EndViewBehavior
End Module
#tag EndModule
