IF NOT "%CONFIG%" == "" GOTO CONFIG
SET CONFIG=deploy\default.json
:CONFIG

IF NOT "%REGION%" == "" GOTO REGION
SET REGION=eu3
:REGION

IF NOT "%SNAPSHOT%" == "" GOTO SNAPSHOT
SET SNAPSHOT=build\initial.snapshot
:SNAPSHOT

IF NOT "%1" == "" GOTO NAME
SET NAME=stu_test
:NAME

spatial upload %NAME%
spatial cloud launch --cluster_region=%REGION% --snapshot=%SNAPSHOT% %NAME% %CONFIG% %NAME%
