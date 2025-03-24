' Updated Demo Script – No Overlap Version

' Create a larger main window to accommodate all controls.
var b as Boolean 
b = XCreateWindow("MainWin", 300, 300, 1200, 900, True, True, True)

if b = true then

    ' Left Column (x ≈ 5)
    XAddTextField("MyTextField", "MainWin", 5, 8, 300, 32)
    Set_FontName("MyTextField", "Calibri")
    Set_FontSize("MyTextField", 18)
    
    XAddListBox("MyListBox", "MainWin", 5, 50, 300, 600)
    Set_FontName("MyListBox", "Calibri")
    Set_FontSize("MyListBox", 16)
    Set_ListBoxRowHeight("MyListBox", 30)
	
    For astr as integer = 0 to 19
        var itemText as String 
        itemText = "This is item " + str(astr)
        print(itemText)
        Listbox_Add("MyListBox", itemText)
    next astr
    
    XAddPopupBox("MyPopup", "MainWin", 5, 660, 300, 100)
    Set_FontName("MyPopup", "Calibri")
    Set_FontSize("MyPopup", 12)
    Popupbox_Add("MyPopup", "Item A")
    Popupbox_Add("MyPopup", "Item B")
    Popupbox_Add("MyPopup", "Item C")
    Set_DropdownHeight("MyPopup", 100)
    
    XAddComboBox("MyCombo", "MainWin", 5, 770, 300, 100)
    Set_FontName("MyCombo", "Calibri")
    Set_FontSize("MyCombo", 12)
    Set_DropdownHeight("MyCombo", 100)
    Popupbox_Add("MyCombo", "Alpha")
    Popupbox_Add("MyCombo", "Beta")
    
    ' Middle Column (x ≈ 320)
    XAddButton("MyButton", "MainWin", 320, 8, 100, 38)
    Set_FontName("MyButton", "Calibri")
    Set_FontSize("MyButton", 12)
    
    XAddDatePicker("MyDate", "MainWin", 320, 60, 200, 30)
	Set_FontName("MyDate", "Calibri")
    Set_FontSize("MyDate", 18)
	
    XAddTimePicker("MyTime", "MainWin", 320, 100, 200, 30)
	Set_FontName("MyTime", "Calibri")
    Set_FontSize("MyTime", 18)
    
    XAddCalendarControl("MyCalendar", "MainWin", 320, 140, 185, 165)
    
    XAddLabel("MyLabel", "MainWin", "Hello, World! <3", 320, 340, 300, 42)
	Set_FontName("MyLabel", "Curlz MT")
    Set_FontSize("MyLabel", 30)
    
    XAddProgressBar("MyProgress", "MainWin", 320, 390, 300, 25)
    //ProgressBar_SetValue("MyProgress", 50)
    //print("Progress value: " + str(ProgressBar_GetValue("MyProgress")))
    
    XAddTabControl("MyTabs", "MainWin", 320, 430, 300, 200)
    
    XAddHScrollBar("MyHScroll", "MainWin", 320, 640, 300, 20)
    XAddVScrollBar("MyVScroll", "MainWin", 630, 430, 20, 200)
    
    XAddUpDownControl("MyUpDown", "MainWin", 320, 670, 50, 25)
    
    ' Right Column (x ≈ 660)
    XAddCheckBox("MyCheckBox", "MainWin", "Check Me", 660, 8, 150, 30)
	Set_FontName("MyCheckBox", "Calibri")
    Set_FontSize("MyCheckBox", 14)
    CheckBox_SetChecked("MyCheckBox", true)
    
    XAddRadioButton("MyRadio1", "MainWin", "Option 1", 660, 50, 150, 30)
		Set_FontName("MyRadio1", "Calibri")
		Set_FontSize("MyRadio1", 14)
    XAddRadioButton("MyRadio2", "MainWin", "Option 2", 810, 50, 150, 30)
		Set_FontName("MyRadio2", "Calibri")
		Set_FontSize("MyRadio2", 14)
    RadioButton_SetChecked("MyRadio1", true)
    
    
    XAddGroupBox("MyGroup", "MainWin", "Group Box", 660, 100, 500, 150)
	XAddLine("MyLine", "MyGroup", 10, 60, 480, 0)
    
    XAddSlider("MySlider", "MainWin", 660, 260, 500, 40)
    Slider_SetValue("MySlider", 75)
    
    XAddColorPicker("MyColorPicker", "MainWin", 660, 310, 200, 40)
    
    XAddChart("MyChart", "MainWin", 660, 360, 500, 200)
    
	//Current: Incomplete control thread creation in plugin - will hard-crash the app but not pose or cause any system/memory leak.
    'XAddMoviePlayer("MyMovie", "MainWin", 660, 570, 500, 200)
    
    XAddHTMLViewer("MyHTML", "MainWin", 660, 570, 500, 250)
    
    ' Status Bar (docked automatically at bottom)
    XAddStatusBar("MyStatus", "MainWin")
    
    ' Tooltip attached to MyColorPicker
    XAddToolTip("MyColorPicker", "Click this button to perform an action.")

end if

var progress as Integer = 0

while true
	progress = progress + 1
	ProgressBar_SetValue("MyProgress", progress)
    print("Progress value: " + str(ProgressBar_GetValue("MyProgress")))
	if progress = 100 then
	   progress = 1
	end if
    DoEvents()
    if ProgressBar_GetValue("MyProgress") = 0 then
        return 0
    end if
    
	Sleep(50)
wend
