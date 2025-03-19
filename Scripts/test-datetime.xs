// Create a DateTime instance for a specific date and time
Dim specificDateTimeHandle As Integer
specificDateTimeHandle = DateTime_Create(2025, 3, 17, 20, 8, 27)

// Output the formatted date and time
Dim specificDateTimeString As String
specificDateTimeString = DateTime_ToString(specificDateTimeHandle)
Print("Specific DateTime: " + specificDateTimeString)

// Retrieve individual components
Dim year As Integer
Dim month As Integer
Dim day As Integer
Dim hour As Integer
Dim minute As Integer
Dim second As Integer

year = DateTime_GetYear(specificDateTimeHandle)
month = DateTime_GetMonth(specificDateTimeHandle)
day = DateTime_GetDay(specificDateTimeHandle)
hour = DateTime_GetHour(specificDateTimeHandle)
minute = DateTime_GetMinute(specificDateTimeHandle)
second = DateTime_GetSecond(specificDateTimeHandle)

Print("Year: " + Str(year))
Print("Month: " + Str(month))
Print("Day: " + Str(day))
Print("Hour: " + Str(hour))
Print("Minute: " + Str(minute))
Print("Second: " + Str(second))

// Clean up by destroying the DateTime instance
DateTime_Destroy(specificDateTimeHandle)
