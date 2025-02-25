Class Pair
  Dim LeftValue As Variant
  Dim RightValue As Variant

  Sub Constructor()
    //Main Constructor
  End Sub

  //Overloaded Constructor
  Sub Constructor( leftValue As Variant, rightValue As Variant )
    self.LeftValue = leftValue
    self.RightValue = rightValue
  End Sub
End Class

var p as new pair
p.LeftValue = "hello "
p.RightValue = "world."
print(p.LeftValue + p.RightValue)

var c as new Pair
c.constructor(30, 40)
print(str(c.leftvalue))
print(str(c.rightvalue))

var vpairs() as Variant
vpairs.add(p)
vpairs.add(c)

//Automatic Typecasting of objects
print(vpairs(0).LeftValue)