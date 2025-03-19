Var handle As Integer
Var filePath As String = "example.bin" ' Change to an actual file path

' Open the binary file
handle = BinaryInputStream_Open(filePath)
If handle = -1 Then
  Print("Failed to open file.")
  Return 0
End If

' Read data from the file
Var byteValue As Integer = BinaryInputStream_ReadByte(handle)
Var shortValue As Integer = BinaryInputStream_ReadShort(handle)
Var longValue As Integer = BinaryInputStream_ReadLong(handle)
Var doubleValue As Double = BinaryInputStream_ReadDouble(handle)
Var textData As String = BinaryInputStream_ReadString(handle, 10) ' Read 10 bytes as a string

' Display results
Print("Byte: " + byteValue.ToString)
Print("Short: " + shortValue.ToString)
Print("Long: " + longValue.ToString)
Print("Double: " + doubleValue.ToString)
Print("Text: " + textData)

' Close the file
BinaryInputStream_Close(handle)
