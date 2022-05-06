#!/bin/bash
ROOT=$(realpath "$PWD/$(dirname "$0")/..")
cd $ROOT
cat resources/provider-header.txt > src/emoji/Provider.cpp 
cat resources/extra_emoji.txt resources/emoji-test.txt > resources/complete-emoji.txt
scripts/emoji_codegen.py resources/complete-emoji.txt resources/shortcodes.txt >> src/emoji/Provider.cpp
cd - > /dev/null
