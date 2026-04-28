<?xml version="1.0" encoding="UTF-8"?>
<Form version="1" dataSource="Order">
  <Title>New Order</Title>
  <Geometry width="420" height="240"/>

  <Widget type="QLabel" name="lblCust" x="20" y="20" width="100" height="24">
    <Property name="text">Customer Id:</Property>
  </Widget>
  <Widget type="QSpinBox" name="spnCustomerId" x="130" y="20" width="120" height="24"
                          binding="CustomerId"/>

  <Widget type="QLabel" name="lblTotal" x="20" y="60" width="100" height="24">
    <Property name="text">Total:</Property>
  </Widget>
  <Widget type="QDoubleSpinBox" name="spnTotal" x="130" y="60" width="160" height="24"
                                binding="Total"/>

  <Widget type="QPushButton" name="btnSave" x="130" y="120" width="100" height="30">
    <Property name="text">Place Order</Property>
  </Widget>

  <Widget type="QLabel" name="lblHint" x="20" y="170" width="380" height="48">
    <Property name="text">Orders over $1000 require manager approval.</Property>
  </Widget>

  <Code><![CDATA[Sub Form_Load()
    Form.New()
End Sub

Sub btnSave_Click()
    If Form.spnTotal <= 0 Then
        MsgBox "Total must be positive."
        Exit Sub
    End If
    If Form.Save() Then
        Dim id = Form.Current.Id
        MsgBox "Order " & id & " placed.  Kicking off approval..."
        Approval.Start()
    End If
End Sub
]]></Code>
</Form>
