<?xml version="1.0" encoding="UTF-8"?>
<Form version="1" dataSource="Customer">
  <Title>Customer Detail</Title>
  <Geometry width="480" height="320"/>

  <Widget type="QLabel" name="lblId" x="20" y="20" width="80" height="24">
    <Property name="text">Id:</Property>
  </Widget>
  <Widget type="QLineEdit" name="txtId" x="110" y="20" width="100" height="24"
                           binding="Id"/>

  <Widget type="QLabel" name="lblName" x="20" y="60" width="80" height="24">
    <Property name="text">Name:</Property>
  </Widget>
  <Widget type="QLineEdit" name="txtName" x="110" y="60" width="340" height="24"
                           binding="Name"/>

  <Widget type="QLabel" name="lblEmail" x="20" y="100" width="80" height="24">
    <Property name="text">Email:</Property>
  </Widget>
  <Widget type="QLineEdit" name="txtEmail" x="110" y="100" width="340" height="24"
                           binding="Email"/>

  <Widget type="QLabel" name="lblBalance" x="20" y="140" width="80" height="24">
    <Property name="text">Balance:</Property>
  </Widget>
  <Widget type="QDoubleSpinBox" name="spnBalance" x="110" y="140" width="160" height="24"
                                binding="Balance"/>

  <Widget type="QCheckBox" name="chkActive" x="110" y="180" width="120" height="24"
                           binding="Active">
    <Property name="text">Active</Property>
  </Widget>

  <Widget type="QPushButton" name="btnNew"    x="20"  y="240" width="80" height="30">
    <Property name="text">New</Property>
  </Widget>
  <Widget type="QPushButton" name="btnLoad"   x="110" y="240" width="80" height="30">
    <Property name="text">Load</Property>
  </Widget>
  <Widget type="QPushButton" name="btnSave"   x="200" y="240" width="80" height="30">
    <Property name="text">Save</Property>
  </Widget>
  <Widget type="QPushButton" name="btnDelete" x="290" y="240" width="80" height="30">
    <Property name="text">Delete</Property>
  </Widget>

  <Code><![CDATA[Sub Form_Load()
    Form.New()
End Sub

Sub btnNew_Click()
    Form.New()
End Sub

Sub btnLoad_Click()
    Dim id = CLng(Form.txtId)
    If id <= 0 Then
        MsgBox "Enter a positive Id first."
        Exit Sub
    End If
    If Form.Load(id) Is Nothing Then
        MsgBox "No customer with Id = " & id
    End If
End Sub

Sub btnSave_Click()
    If Trim(Form.txtName) = "" Then
        MsgBox "Name is required."
        Exit Sub
    End If
    If Form.Save() Then
        MsgBox "Saved as Id " & Form.Current.Id
    End If
End Sub

Sub btnDelete_Click()
    If Form.Current Is Nothing Then Exit Sub
    If Form.Delete() Then
        MsgBox "Deleted."
        Form.New()
    End If
End Sub
]]></Code>
</Form>
