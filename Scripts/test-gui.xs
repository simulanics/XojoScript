
var b as Boolean 
b = XCreateWindow("MainWin", 300, 300, 1024, 768, True, True, True)

if b = true then

    XAddTextField("MyTextField", "MainWin", 5, 8, 300, 32)
    Set_FontName("MyTextField", "Calibri")
    Set_FontSize("MyTextField", 18)

    XAddButton("MyButton", "MainWin", 310, 5, 100, 38)
    Set_FontName("MyButton", "Calibri")
    Set_FontSize("MyButton", 12)

    XAddListBox("MyListBox", "MainWin", 5, 50, 300, 680)
    Set_FontName("MyListBox", "Calibri")
    Set_FontSize("MyListBox", 16)
    Set_ListboxRowHeight("MyListBox", 30)

    For astr as integer = 0 to 19
        var itemText as String 
        itemText = ("This is item " + str(astr))
        print(itemText)
        Listbox_Add("MyListBox", itemText)
    next astr

    XAddPopupBox("MyPopup", "MainWin", 310, 50, 250, 300)
    Set_FontName("MyPopup", "Calibri")
    Set_FontSize("MyPopup", 12)

    var tt as string = "This is item 1"
    PopupBox_Add("MyPopup", tt)
    PopupBox_Add("MyPopup", "Item 2 lives here!")
    PopupBox_Add("MyPopup", "Item 3 lives here!")
    Set_DropdownHeight("MyPopup", 300)

    XAddComboBox("MyCombo", "MainWin", 310, 95, 250, 200)
    Set_FontName("MyCombo", "Calibri")
    Set_FontSize("MyCombo", 12)
    Set_DropdownHeight("MyPopup", 150)

    PopupBox_Add("MyCombo", "Item 2 lives here!")
    PopupBox_Add("MyCombo", "Item 3 lives here!")

    //XRefresh(true)

end if


while true
 DoEvents()
wend