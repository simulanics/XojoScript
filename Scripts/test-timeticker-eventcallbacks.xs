' ***********************************************************************
' Demo XojoScript for using the TimeTicker Plugin (Trigger event only)
'
' This script demonstrates:
'   - Creating a TimeTicker instance via the plugin function CreateTimeTicker
'   - Attaching a callback for the "Trigger" event using AddressOf and AddHandler functions.
'
' The event target identifier format is:
'   "plugin:<handle>:Trigger"
'
' ***********************************************************************

' -----------------------------------------------------------------------
' Callback function for the "Trigger" event.
' It receives the current time (as a string) when triggered.
' -----------------------------------------------------------------------
Sub TimerTrigger(currentTime As String)
  Print("Trigger event fired at: " + currentTime)
End Sub

' -----------------------------------------------------------------------
' Main program execution:
'   - Create a TimeTicker instance.
'   - Attach the Trigger callback.
'   - Loop indefinitely so events can occur.
' -----------------------------------------------------------------------
Dim tickerHandle As Integer

' Create a new TimeTicker instance using the plugin's CreateTimeTicker function.
tickerHandle = CreateTimeTicker()

' Attach the Trigger callback.
' The target string is in the format "plugin:<handle>:Trigger"
AddHandler("plugin:" + Str(tickerHandle) + ":Trigger", AddressOf(TimerTrigger))

Print("Created TimeTicker instance. Handle = " + Str(tickerHandle))

' Keep the script running indefinitely so that timer events continue.
While True
   DoEvents()
Wend
