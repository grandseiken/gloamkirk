SET PROJECT=alpha_zebra_pizza_956

IF NOT "%CONFIG%" == "" GOTO CONFIG
SET CONFIG=deploy\default.json
:CONFIG

IF NOT "%CLUSTER%" == "" GOTO CLUSTER
SET CLUSTER=eu3-prod
:CLUSTER

IF NOT "%SNAPSHOT%" == "" GOTO SNAPSHOT
SET SNAPSHOT=build\initial.snapshot
:SNAPSHOT

IF NOT "%1" == "" GOTO NAME
SET NAME=stu_test
:NAME

spatial upload %NAME%
spatial deployment launch --cluster=%CLUSTER% --snapshot=%SNAPSHOT% %PROJECT% %NAME% %CONFIG% %NAME%
