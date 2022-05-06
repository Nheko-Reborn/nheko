# Updating emoji

1. Get the latest emoji-test.txt from here: https://unicode.org/Public/emoji/
2. Overwrite the existing resources/emoji-test.txt with the new one
3. Run `./scripts/emoji_codegen.py resources/emoji-test.txt resources/shortcodes.txt` and replace the current tail of src/emoji/Provider.cpp with the new output
4. `make lint`
5. Compile and test


