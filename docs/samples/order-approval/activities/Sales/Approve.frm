<?xml version="1.0" encoding="UTF-8"?>
<Form version="1">
  <Title>Manager Approval</Title>
  <Geometry width="380" height="200"/>

  <Widget type="QLabel" name="lblPrompt" x="20" y="20" width="340" height="24">
    <Property name="text">Approve this order?</Property>
  </Widget>

  <Widget type="QLabel" name="lblName" x="20" y="60" width="100" height="24">
    <Property name="text">Approver:</Property>
  </Widget>
  <Widget type="QLineEdit" name="txtApprover" x="130" y="60" width="220" height="24"/>

  <Widget type="QPushButton" name="btnApprove" x="130" y="120" width="100" height="30">
    <Property name="text">Approve</Property>
  </Widget>
  <Widget type="QPushButton" name="btnReject"  x="240" y="120" width="100" height="30">
    <Property name="text">Reject</Property>
  </Widget>

  <Code><![CDATA[Sub btnApprove_Click()
    If Trim(Form.txtApprover) = "" Then
        MsgBox "Enter your name."
        Exit Sub
    End If
    Form.Accept()         ' resume the workflow with awaiting_human -> next
End Sub

Sub btnReject_Click()
    Form.Reject()         ' aborts the workflow
End Sub
]]></Code>
</Form>
