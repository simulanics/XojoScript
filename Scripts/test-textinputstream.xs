Var handle As Integer
Var filePath As String = "example.txt"

' Open the text file
handle = TextInputStream_Open(filePath)
If handle = -1 Then
  Print("Failed to open file.")
  Return 0
End If

' Read and display each line
While TextInputStream_EOF(handle) = 0
  Var line As String = TextInputStream_ReadLine(handle)
  Print(line)
Wend

' Close the file
TextInputStream_Close(handle)
