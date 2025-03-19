Dim path As String = "testfile.txt"

// Check if the file exists
If FolderItem_Exists(path) Then
    Print "File exists!"
Else
    Print "File does not exist."
End If

// Create a new directory
Dim dirPath As String = "./TestFolder"
If FolderItem_CreateDirectory(dirPath) Then
    Print "Directory created successfully!"
Else
    Print "Failed to create directory."
End If

// Get file size
Dim fileSize As Integer = FolderItem_Size(path)
Print "File size: " + Str(fileSize) + " bytes"

// Check if it's a directory
If FolderItem_IsDirectory(path) Then
    Print "This is a directory."
Else
    Print "This is a file."
End If

// Get the full path
Dim fullPath As String = FolderItem_GetPath(path)
Print "Full path: " + fullPath

// Get file permissions (Unix-style)
Dim permissions As Integer = FolderItem_GetPermission(path)
Print "File permissions: " + Str(permissions)

// Set new file permissions (only works on Unix-based systems)
Dim newPermissions As Integer = 644
If FolderItem_SetPermission(path, newPermissions) Then
    Print "Permissions updated successfully!"
Else
    Print "Failed to change permissions."
End If

// Get the URL version of the path
Dim urlPath As String = FolderItem_URLPath(path)
Print "URL Path: " + urlPath

// Get the shell-safe version of the path
Dim shellPath As String = FolderItem_ShellPath(path)
Print "Shell Path: " + shellPath

// Delete the file
If FolderItem_Delete(path) Then
    Print "File deleted successfully!"
Else
    Print "Failed to delete file."
End If
