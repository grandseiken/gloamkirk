IF NOT "%CONFIG%" == "" GOTO CONFIG
SET CONFIG=deploy\default.json
:CONFIG

IF NOT "%SNAPSHOT%" == "" GOTO SNAPSHOT
SET SNAPSHOT=build\initial.snapshot
:SNAPSHOT

spatial local launch --snapshot=%SNAPSHOT% %CONFIG%
