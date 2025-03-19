Var handle As Integer
Var filePath As String = "example.bin"

' Create and open a binary file for writing (overwrite)
handle = BinaryOutputStream_Create(filePath, False)
If handle = -1 Then
  Print("Failed to create file.")
  Return 0
End If

' Write binary data
BinaryOutputStream_WriteByte(handle, 255)
BinaryOutputStream_WriteShort(handle, 32000)
BinaryOutputStream_WriteLong(handle, 123456789)
BinaryOutputStream_WriteDouble(handle, 3.14159)
BinaryOutputStream_WriteString(handle, "Hello")

' Flush and close the file
BinaryOutputStream_Flush(handle)
BinaryOutputStream_Close(handle)

Print("Binary file written successfully.")
