//%attributes = {}
C_LONGINT:C283($1; $keyType)
C_TEXT:C284($2; $context)

$keyType:=$1
$context:=$2

C_OBJECT:C1216($params)
$params:=JSON Parse:C1218($context; Is object:K8:27)

CALL FORM:C1391($params.window; $params.method; $keyType)