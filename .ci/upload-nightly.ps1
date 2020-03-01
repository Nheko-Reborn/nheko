$file = "nheko_win_64.zip"
$fileName = "nheko-${env:APPVEYOR_REPO_BRANCH}-${env:APPVEYOR_REPO_COMMIT}-win64.zip"

$response = Invoke-RestMethod -uri "https://matrix.neko.dev/_matrix/media/r0/upload?filename=$fileName" -Method Post -Infile "$file" -ContentType 'application/x-compressed' -Headers @{"Authorization"="Bearer ${env:MATRIX_ACCESS_TOKEN}"}

$txId = [DateTimeOffset]::Now.ToUnixTimeSeconds()
$fileSize = (Get-Item $file).Length
$body = @{
  "body" = "${fileName}"
  "filename"= "${fileName}"
  "info" = @{
    "mimetype" = "application/x-compressed"
    "size" = ${fileSize}
  }
  "msgtype" = "m.file"
  "url" = ${response}.content_uri
} | ConvertTo-Json
$room = "!TshDrgpBNBDmfDeEGN:neko.dev"

Invoke-RestMethod -uri "https://matrix.neko.dev/_matrix/client/r0/rooms/${room}/send/m.room.message/${txid}" -Method Put -Body "$body" -ContentType 'application/json' -Headers @{"Authorization"="Bearer ${env:MATRIX_ACCESS_TOKEN}"}

