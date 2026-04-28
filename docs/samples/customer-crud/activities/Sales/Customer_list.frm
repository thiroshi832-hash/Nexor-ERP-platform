<?xml version="1.0" encoding="UTF-8"?>
<Form version="1">
  <Title>Customers</Title>
  <Geometry width="520" height="380"/>

  <Widget type="QLabel" name="lblHeader" x="20" y="10" width="480" height="24">
    <Property name="text">Active customers, ordered by name:</Property>
  </Widget>
  <Widget type="QPlainTextEdit" name="txtList" x="20" y="40" width="480" height="280">
    <Property name="readOnly">true</Property>
  </Widget>
  <Widget type="QPushButton" name="btnRefresh" x="20" y="335" width="100" height="30">
    <Property name="text">Refresh</Property>
  </Widget>

  <Code><![CDATA[Sub Form_Load()
    Refresh()
End Sub

Sub btnRefresh_Click()
    Refresh()
End Sub

Sub Refresh()
    Dim active = From c In Customer _
                 Where c.Active _
                 OrderBy c.Name _
                 Select c

    Dim out = "  Id     Name                           Balance" & Chr(10) & _
              "  -----  ------------------------------ ----------" & Chr(10)
    For Each c In active
        out = out & "  " & Right("    " & c.Id, 5) & "  " & _
              Left(c.Name & "                              ", 30) & " " & _
              Right("            $" & Round(c.Balance, 2), 10) & Chr(10)
    Next c
    Form.txtList = out
End Sub
]]></Code>
</Form>
