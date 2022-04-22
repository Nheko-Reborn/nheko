#!/bin/bash
ROOT=$(realpath "$PWD/$(dirname "$0")/..")
cd $ROOT
cat resources/provider-header.txt > src/emoji/Provider.cpp 

scripts/emoji_codegen.py resources/emoji-test.txt resources/shortcodes.txt >> src/emoji/Provider.cpp
