Dim handle As Integer
Dim size As Integer = 16
handle = MemoryBlock_Create(size)
If handle = -1 Then
  Print("Failed to create MemoryBlock.")
  Return 0
End If

MemoryBlock_WriteByte(handle, 0, 255)
MemoryBlock_WriteShort(handle, 1, 32000)
MemoryBlock_WriteLong(handle, 4, 123456789)
MemoryBlock_WriteDouble(handle, 8, 3.14159)

Dim byteVal As Integer = MemoryBlock_ReadByte(handle, 0)
Dim shortVal As Integer = MemoryBlock_ReadShort(handle, 1)
Dim longVal As Integer = MemoryBlock_ReadLong(handle, 4)
Dim doubleVal As Double = MemoryBlock_ReadDouble(handle, 8)

Print("Byte: " + Str(byteVal))
Print("Short: " + Str(shortVal))
Print("Long: " + Str(longVal))
Print("Double: " + Str(doubleVal))

MemoryBlock_Resize(handle, 32)
Print("Resized MemoryBlock to 32 bytes")

Dim copyHandle As Integer = MemoryBlock_Create(32)
MemoryBlock_CopyMemory(copyHandle, 0, handle, 0, 16)
Print("Copied 16 bytes to new MemoryBlock")

MemoryBlock_Destroy(handle)
MemoryBlock_Destroy(copyHandle)
Print("MemoryBlocks destroyed.")
