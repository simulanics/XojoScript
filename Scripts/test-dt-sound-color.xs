	//Verse 1
    // Frequencies (in Hz) and durations (in ms) for the melody.
    var frequencies() As Integer = Array(330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 330, 294, 294)
    var durations() As Integer = Array(300, 300, 600, 600, 600, 600, 600, 600, 600, 300, 300, 600, 600, 600, 900)
    
	//Verse 2/4
	var frequencies2() As Integer = Array(330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 294, 262, 262)
	var durations2() As Integer = Array(300, 300, 600, 600, 600, 600, 600, 600, 600, 300, 300, 600, 600, 600, 900)

	//Verse 3
	var frequencies3() As Integer = Array(294, 294, 330, 262, 294, 330, 349, 330, 262, 294, 330, 349, 330, 294, 262, 294, 196)
	var durations3() As Integer = Array(300, 300, 600, 600, 600, 600, 600, 600, 300, 300, 600, 600, 600, 600, 900, 600, 900)

	Sub PlayVerse(freqs as variant, durs as variant)
		For i As Integer = 0 To freqs.Count() - 1
			Beep(freqs(i), durs(i))
			var NR as new Random
			var r as Integer = nr.InRange(0,255)
			var g as Integer = nr.InRange(0,255)
			var b as Integer = nr.InRange(0,255)
			var clr as String = RGBtoHEX(r, g, b)
			PrintColor(clr + " ", clr)
			Print(GetCurrentDate() + " - " + GetCurrentTime())
			DoEvents()
			//Sleep(50) // brief pause between notes
		Next
	End Sub


    Print("Turn your volume up! Ode to Joy!")
	PlayVerse(frequencies, durations)
	PlayVerse(frequencies2, durations2)
	PlayVerse(frequencies3, durations3)
	PlayVerse(frequencies2, durations2)
