<?xml version="1.0" encoding="UTF-8"?>
<NexorProject version="1">
    <Meta>
        <Title>Order Approval</Title>
        <Id>OrderApproval</Id>
        <Description>BPMN-driven order-approval workflow with a Choice gateway and a HumanTask.</Description>
        <Author>nexor docs</Author>
        <Created>2026-04-29T10:00:00</Created>
    </Meta>
    <Events/>
    <AtomicActivities>
        <Activity id="Sales" file="activities/Sales/Sales.aba"/>
    </AtomicActivities>
    <ProcessActivities>
        <Process id="Approval" file="processes/Approval/Approval.bpmn"/>
    </ProcessActivities>
    <Sheets>
        <Sheet id="Order" file="sheets/Order/Order.sht"/>
    </Sheets>
    <Reports/>
    <Resources/>
</NexorProject>
