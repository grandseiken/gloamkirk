SET PROJECT=alpha_zebra_pizza_956
SET CONFIG=deploy\default.json
SET CLUSTER=eu1-prod
SET SNAPSHOT=build\initial.snapshot

spatial upload %1
spatial deployment launch --cluster=%CLUSTER% --snapshot=%SNAPSHOT% %PROJECT% %1 %CONFIG% %1
