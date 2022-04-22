#!/usr/bin/env python3

import sys
import re
from unidecode import unidecode
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
    if len(sys.argv) < 3:
        print('usage: emoji_codegen.py /path/to/emoji-test.txt /path/to/shortcodes.txt')
        sys.exit(1)

    filename = sys.argv[1]
    shortcodefilename = sys.argv[2]

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
    shortcodeDict = {} 
    # for my sanity - this strips newlines
    for line in open(shortcodefilename, 'r', encoding="utf8"): 
        longname, shortname = line.strip().split(':')
        shortcodeDict[longname] = shortname
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
        if qualification != 'fully-qualified':
            continue


        char, name = re.match(r'^(\S+) E\d+\.\d+ (.*)$', charAndName).groups()
        #TODO: Handle skintone modifiers in a sane way
        if name in shortcodeDict: 
            # TODO: this duplicates emoji
            categories[current_category].append(Emoji(code, shortcodeDict[name]))
        
        if name.endswith(' face'): 
            name = name[:-5]
        elif name.endswith(' button'): 
            name = name[:-7] 
        else: 
            matchobj = re.match(r'^flag: (.*)$', name) 
            if matchobj: 
                country, = matchobj.groups() 
                name = country + " flag"
        name = name.replace(" ", "_")
        name = name.replace("“", "")
        name = name.replace("”", "")
        name = name.replace(":", "")
        name = name.replace("-", "_")
        name = re.sub(r'_{2,}', '_', name) 
        name = name.lower()
        name = unidecode(name)
        categories[current_category].append(Emoji(code, name))

    # Use xclip to pipe the output to clipboard.
    # e.g ./codegen.py emoji.json | xclip -sel clip
    # alternatively - delete the var from src/emoji/Provider.cpp, and do ./codegen.py emojis shortcodes >> src/emoji/Provider.cpp
    generate_qml_list(people=people, nature=nature, food=food, activity=activity, travel=travel, objects=objects, symbols=symbols, flags=flags)
