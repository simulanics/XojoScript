' Module: MathUtilities
Module MathUtilities
  Public Function Multiply(a As Integer, b As Integer) As Integer
    Return a * b
  End Function
End Module

' Class: Person
Class Person
  Var Name As String
  
  ' Constructor
  Sub Constructor(newName As String)
    Name = newName
  End Sub
  
  ' Method to display a greeting
  Public Sub Greet()
    Print("Hello, my name is " + Name)
  End Sub
End Class

' Main Script
Dim x As Integer = 5
Dim y As Integer = 10
Print("Sum: " + Str(x + y))

If x < y Then
  Print("x is less than y")
Else
  Print("x is not less than y")
End If

For i = 1 To 3
  Print("Loop: " + Str(i))
Next

Dim purple As Color = &c800080
Print("Color literal for purple: " + Str(purple))

' Use built-in math functions
Dim result As Double = Pow(2, 3) + Sqrt(16)
Print("Result: " + Str(result))

' Using the Random class
Dim rndInstance As New Random
Print("Random Number (1 to 100): " + Str(rndInstance.InRange(1, 100)))

' Using a module function
Print("Multiply 6 * 7 = " + Str(MathUtilities.Multiply(6, 7)))

' Create and use a class instance
Dim person As New Person
person.constructor("Alice")
person.Greet()