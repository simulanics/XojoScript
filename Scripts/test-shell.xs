Dim shellID As Integer
Dim command As String
Dim success As Boolean
Dim result As String
Dim exitCode As Integer

// Create a new shell instance
shellID = Shell_Create

If shellID > 0 Then
    // Define the command (Windows example: "dir", macOS/Linux: "ls")
    '#If TargetWindows Then
        command = "dir"
    '#Else
        command = "ls"
    '#EndIf
    
    // Execute the command with a 5000ms (5 seconds) timeout
    success = Shell_Execute(shellID, command, 5000)

    If success Then
        // Get the command output
        result = Shell_Result(shellID)
        Print "Command Output: " + result
        
        // Get the exit code
        exitCode = Shell_ExitCode(shellID)
        Print "Exit Code: " + Str(exitCode)
    Else
        Print "Failed to execute command."
    End If

    // Destroy the shell instance
    Shell_Destroy(shellID)
Else
    Print "Failed to create Shell instance."
End If
