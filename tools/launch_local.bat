SET CONFIG=deploy\default.json
SET SNAPSHOT=build\initial.snapshot

spatial local launch --snapshot=%SNAPSHOT% %CONFIG%
