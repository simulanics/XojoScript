Var handle As Integer
Var filePath As String = "example.txt"

' Create and open a text file (overwrite)
handle = TextOutputStream_Create(filePath, False)
If handle = -1 Then
  Print("Failed to create file.")
  Return 0
End If

' Write text data
TextOutputStream_WriteLine(handle, "Hello, world!")
TextOutputStream_WriteLine(handle, "This is a test.")
TextOutputStream_Write(handle, "More text without newline.")

' Flush and close the file
TextOutputStream_Flush(handle)
TextOutputStream_Close(handle)

Print("Text file written successfully.")
