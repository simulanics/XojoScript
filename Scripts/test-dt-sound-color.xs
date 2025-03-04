Sub Main()

    print("Turn your volume up! Ode to Joy!")
    var tr as Boolean

    // Frequencies (in Hz) and durations (in ms) for the melody.
    var frequencies() As Integer = Array(330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 330, 294, 294)
    var durations() As Integer = Array(300, 300, 600, 600, 600, 600, 600, 600, 600, 300, 300, 600, 600, 600, 900)
    
    For i As Integer = 0 To frequencies.Count() - 1
      var t as integer = frequencies(i)
      var u as integer = frequencies(i)
      tr = Beep(frequencies(i), durations(i))
      //Sleep(50) // brief pause between notes
      var NR as new Random
      var r as Integer = nr.InRange(0,255)
      var g as Integer = nr.InRange(0,255)
      var b as Integer = nr.InRange(0,255)
      var clr as String = RGBtoHEX(r, g, b)
      printcolor(clr + " ", clr)
      print(GetCurrentDate() + " - " + GetCurrentTime())
      DoEvents()
    Next
 
End Sub
