#!/usr/bin/env python3

import sys
import json

from jinja2 import Template


class Emoji(object):
    def __init__(self, code, shortname, category, order):
        self.code = ''.join(list(map(code_to_bytes, code.split('-'))))
        self.shortname = shortname
        self.category = category
        self.order = int(order)


def code_to_bytes(codepoint):
    '''
    Convert hex unicode codepoint to hex byte array.
    '''
    bytes = chr(int(codepoint, 16)).encode('utf-8')

    return str(bytes)[1:].strip("'")


def generate_code(emojis, category):
    tmpl = Template('''
const QList<Emoji> EmojiProvider::{{ category }} = {
    {%- for e in emoji %}
        Emoji{QString::fromUtf8("{{ e.code }}"), "{{ e.shortname }}"},
    {%- endfor %}
};
    ''')

    d = dict(category=category, emoji=emojis)
    print(tmpl.render(d))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('usage: emoji_codegen.py /path/to/emoji.json')
        sys.exit(1)

    filename = sys.argv[1]
    data = {}

    with open(filename, 'r') as filename:
        data = json.loads(filename.read())

    emojis = []

    for emoji_name in data:
        tmp = data[emoji_name]

        l = len(tmp['unicode'].split('-'))

        if l > 1 and tmp['category'] == 'people':
            continue

        emojis.append(
            Emoji(
                tmp['unicode'],
                tmp['shortname'],
                tmp['category'],
                tmp['emoji_order']
            )
        )

    emojis.sort(key=lambda x: x.order)

    people = list(filter(lambda x: x.category == "people", emojis))
    nature = list(filter(lambda x: x.category == "nature", emojis))
    food = list(filter(lambda x: x.category == "food", emojis))
    activity = list(filter(lambda x: x.category == "activity", emojis))
    travel = list(filter(lambda x: x.category == "travel", emojis))
    objects = list(filter(lambda x: x.category == "objects", emojis))
    symbols = list(filter(lambda x: x.category == "symbols", emojis))
    flags = list(filter(lambda x: x.category == "flags", emojis))

    # Use xclip to pipe the output to clipboard.
    # e.g ./codegen.py emoji.json | xclip -sel clip
    generate_code(people, 'people')
    generate_code(nature, 'nature')
    generate_code(food, 'food')
    generate_code(activity, 'activity')
    generate_code(travel, 'travel')
    generate_code(objects, 'objects')
    generate_code(symbols, 'symbols')
    generate_code(flags, 'flags')
