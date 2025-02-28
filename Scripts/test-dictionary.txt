' Dictionary Class Example Test Script

'----------------------------------------------------------------
' Dictionary class – stores key-value pairs using  dynamic arrays.
'----------------------------------------------------------------
Class Dictionary

  ' Property: data is an array of key–value pairs.
  ' Each element is itself an array of two elements:
  '   [0] = key (a String)
  '   [1] = value (a Variant)
  Var data() As String
  Var vals() As Variant

  ' Constructor: initializes an empty dictionary.
 Sub Constructor()
    data = Array() ' Create an empty dynamic array.
    vals = Array()
 End Sub

 ' Method: Count – returns the total number of items in the dictionary.
  Function Count() As Integer
    Return data.count()
  End Function

  ' Method: SetItem – adds or updates a key-value pair.
  Sub SetItem(key As String, value As Variant)
    Dim found As Boolean = false
    For i As Integer = 0 To self.Count() - 1
     If data(i) = key Then
        vals(i) = value
        found = true
     End If
    Next

    If found = false Then //check not
       data.add(key)
       vals.add(value)
    End if
  End Sub

  ' Method: GetItem – returns the value associated with the key (or nil if not found).
  Function GetItem(key As String) As String
    For i As Integer = 0 To data.count() - 1
      If data(i) = key Then
         Return vals(i)
      End If
    Next
    Return "nil"
  End Function

 

  ' Method: Keys – returns an array of all keys (as strings).
  Function Keys() As String
     Return data
  End Function

End Class

'----------------------------------------------------------------
' Test code for Dictionary
'----------------------------------------------------------------

' Create a new Dictionary instance.
Dim dt As New Dictionary
dt.constructor()
' Set some key–value pairs.
dt.SetItem("name", "Alice")
dt.SetItem("age", 30)
dt.SetItem("city", "Wonderland")

' Print individual values.
print("Name: " + dt.GetItem("name"))
print("Age: " + str(dt.GetItem("age")))
print("City: " + dt.GetItem("city"))
print("Total items: " + str(dt.Count()))

' Get and iterate through all keys.
Dim keys() As String = dt.Keys()
For i As Integer = 0 To keys.count() - 1
   print("Key: " + keys(i) + ", Value: " + str(dt.GetItem(keys(i))))
Next

print("Dictionary test completed.")