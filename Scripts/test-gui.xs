' Updated Demo Script – No Overlap Version

' Create a larger main window to accommodate all controls.
var b as Boolean 
b = XCreateWindow("MainWin", 300, 300, 1400, 900, True, True, True)

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
    XAddTimePicker("MyTime", "MainWin", 320, 100, 200, 30)
    
    XAddCalendarControl("MyCalendar", "MainWin", 320, 140, 300, 200)
    
    XAddLabel("MyLabel", "MainWin", "Hello, World!", 320, 350, 200, 30)
    
    XAddProgressBar("MyProgress", "MainWin", 320, 390, 300, 25)
    ProgressBar_SetValue("MyProgress", 50)
    print("Progress value: " + str(ProgressBar_GetValue("MyProgress")))
    
    XAddTabControl("MyTabs", "MainWin", 320, 430, 300, 200)
    
    XAddHScrollBar("MyHScroll", "MainWin", 320, 640, 300, 20)
    XAddVScrollBar("MyVScroll", "MainWin", 630, 430, 20, 200)
    
    XAddUpDownControl("MyUpDown", "MainWin", 320, 670, 50, 25)
    
    ' Right Column (x ≈ 650)
    XAddCheckBox("MyCheckBox", "MainWin", "Check Me", 650, 8, 150, 30)
    CheckBox_SetChecked("MyCheckBox", true)
    
    XAddRadioButton("MyRadio1", "MainWin", "Option 1", 650, 50, 150, 30)
    XAddRadioButton("MyRadio2", "MainWin", "Option 2", 810, 50, 150, 30)
    RadioButton_SetChecked("MyRadio1", true)
    
    XAddLine("MyLine", "MainWin", 650, 90, 500, 2)
    
    XAddGroupBox("MyGroup", "MainWin", "Group Box", 650, 100, 500, 150)
    
    XAddSlider("MySlider", "MainWin", 650, 260, 500, 40)
    Slider_SetValue("MySlider", 75)
    
    XAddColorPicker("MyColorPicker", "MainWin", 650, 310, 200, 40)
    
    XAddChart("MyChart", "MainWin", 650, 360, 500, 200)
    
    XAddMoviePlayer("MyMovie", "MainWin", 650, 570, 500, 200)
    
    XAddHTMLViewer("MyHTML", "MainWin", 650, 780, 500, 100)
    
    ' Status Bar (docked automatically at bottom)
    XAddStatusBar("MyStatus", "MainWin")
    
    ' Tooltip attached to MyColorPicker
    XAddToolTip("MyColorPicker", "Click this button to perform an action.")

end if

while true
    DoEvents()
wend
