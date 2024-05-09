#!/bin/bash
ROOT=$(realpath "$PWD/$(dirname "$0")/..")
cd $ROOT
cat resources/provider-head.txt > src/emoji/Provider.cpp 
cat resources/extra_emoji.txt resources/emoji-test.txt > resources/complete-emoji.txt
scripts/emoji_codegen.py impl resources/complete-emoji.txt resources/shortcodes.txt >> src/emoji/Provider.cpp
scripts/emoji_codegen.py header resources/complete-emoji.txt resources/shortcodes.txt > src/emoji/Provider.h
cd - > /dev/null
