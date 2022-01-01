#!/usr/bin/env python3

import sys
import re

from jinja2 import Template


class Emoji(object):
    def __init__(self, code, shortname):
        self.code = ''.join(['\\U'+c.rjust(8, '0') for c in code.strip().split(' ')])
        self.shortname = shortname

def generate_qml_list(**kwargs):
    tmpl = Template('''
const QVector<Emoji> emoji::Provider::emoji = {
    {%- for c in kwargs.items() %}
    // {{ c[0].capitalize() }}
    {%- for e in c[1] %}
    Emoji{QStringLiteral(u"{{ e.code }}"), QStringLiteral(u"{{ e.shortname }}"), emoji::Emoji::Category::{{ c[0].capitalize() }}},
    {%- endfor %}
    {%- endfor %}
};
    ''')
    d = dict(kwargs=kwargs)
    print(tmpl.render(d))

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('usage: emoji_codegen.py /path/to/emoji-test.txt')
        sys.exit(1)

    filename = sys.argv[1]

    people = []
    nature = []
    food = []
    activity = []
    travel = []
    objects = []
    symbols = []
    flags = []

    categories = {
        'Smileys & Emotion': people,
        'People & Body': people,
        'Animals & Nature': nature,
        'Food & Drink': food,
        'Travel & Places': travel,
        'Activities': activity,
        'Objects': objects,
        'Symbols': symbols,
        'Flags': flags
    }

    current_category = ''
    for line in open(filename, 'r', encoding="utf8"):
        if line.startswith('# group:'):
            current_category = line.split(':', 1)[1].strip()

        if not line or line.startswith('#'):
            continue

        segments = re.split(r'\s+[#;] ', line.strip())
        if len(segments) != 3:
            continue

        code, qualification, charAndName = segments

        # skip unqualified versions of same unicode
        if qualification == 'unqualified':
            continue

        if qualification == 'component':
            continue

        char, name = re.match(r'^(\S+) E\d+\.\d+ (.*)$', charAndName).groups()

        categories[current_category].append(Emoji(code, name))

    # Use xclip to pipe the output to clipboard.
    # e.g ./codegen.py emoji.json | xclip -sel clip
    generate_qml_list(people=people, nature=nature, food=food, activity=activity, travel=travel, objects=objects, symbols=symbols, flags=flags)
