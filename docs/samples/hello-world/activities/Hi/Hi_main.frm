<?xml version="1.0" encoding="UTF-8"?>
<Form version="1" id="">
    <Geometry x="100" y="100" width="320" height="160"/>
    <Title>Hello</Title>
    <Widgets>
        <Widget type="QLabel" name="lblPrompt" x="20" y="20" width="80" height="24">
            <Property name="text">Your name:</Property>
        </Widget>
        <Widget type="QLineEdit" name="txtName" x="110" y="20" width="180" height="24">
            <Property name="text">friend</Property>
        </Widget>
        <Widget type="QPushButton" name="btnHello" x="110" y="80" width="100" height="30">
            <Property name="text">Hello</Property>
        </Widget>
    </Widgets>
    <Code><![CDATA[Sub btnHello_Click()
    MsgBox "Hello, " & Form.txtName & "!"
End Sub
]]></Code>
</Form>
