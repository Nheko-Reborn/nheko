#!/bin/sh

export PATH="${PATH}:/usr/local/bin/"

file=$(find artifacts/ -type f -exec basename {} \;)

uri=$(curl -H "Authorization: Bearer ${MATRIX_ACCESS_TOKEN}" -H "Content-Type: application/x-compressed" -X POST --data-binary "@artifacts/${file}" "https://matrix.neko.dev/_matrix/media/r0/upload?filename=${file}" | python3 -c "import sys, json; print(json.load(sys.stdin)['content_uri'])")
echo "Uploaded to ${uri}"

curl -H "Authorization: Bearer ${MATRIX_ACCESS_TOKEN}" -H "Content-Type: application/json" -X PUT -d "{ \"body\": \"${file}\", \"filename\": \"${file}\", \"info\": { \"mimetype\": \"application/x-compressed\", \"size\": $(wc -c < artifacts/${file}) }, \"msgtype\": \"m.file\", \"url\": \"${uri}\" }" "https://matrix.neko.dev/_matrix/client/r0/rooms/${ROOM}/send/m.room.message/${RANDOM}"
