var db as Integer = OpenDatabase("mydb.sqlite")

ExecuteSQL(db, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)")

var stmt as Integer = PrepareStatement(db, "INSERT INTO users (name) VALUES (?)")
BindString(stmt, 1, "John Doe")
ExecutePrepared(stmt)
FinalizeStatement(stmt)

stmt = PrepareStatement(db, "SELECT id, name FROM users")
If MoveToFirstRow(stmt) Then
    While Not MoveToNextRow(stmt)
        var id as Integer = ColumnInteger(stmt, 0)
        var name as String = ColumnString(stmt, 1)
        Print("ID: " + Str(id) + ", Name: " + name)
    Wend
End If
FinalizeStatement(stmt)
