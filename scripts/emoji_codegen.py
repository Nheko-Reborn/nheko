#!/usr/bin/env python3

import sys
import re
from unidecode import unidecode
from jinja2 import Template


class Emoji(object):
    def __init__(self, code, shortname, unicodename):
        self.code = ''.join(['\\U'+c.rjust(8, '0') for c in code.strip().split(' ')])
        self.shortname = shortname
        self.unicodename = unicodename

def generate_qml_list(**kwargs):
    tmpl = Template('''
const std::array<Emoji, {{ sum([len(c[1]) for c in kwargs.items()]) }}> emoji::Provider::emoji = {
    {%- for c in kwargs.items() %}
    // {{ c[0].capitalize() }}
    {%- for e in c[1] %}
    Emoji{QStringLiteral(u"{{ e.code }}"), QStringLiteral(u"{{ e.shortname }}"), QStringLiteral(u"{{ e.unicodename }}"), emoji::Emoji::Category::{{ c[0].capitalize() }}},
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
        'Flags': flags,
        'Component': symbols
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
        shortname = name
        # until skin tone is handled, keep them around
        ## discard skin tone variants for sanity
        # if "skin tone" in name and qualification != 'component': 
        #    continue
        # if qualification == 'component' and not "skin tone" in name:
        #    continue
        #TODO: Handle skintone modifiers in a sane way
        basicallyTheSame = False
        if code in shortcodeDict:
            shortname = shortcodeDict[code]
        else:
            shortname = shortname.lower()
            if shortname.endswith(' (blood type)'):
                shortname = shortname[:-13]
            if shortname.endswith(': red hair'):
                shortname = "red_haired_" + shortname[:-10]
            if shortname.endswith(': curly hair'):
                shortname = "curly_haired_" + shortname[:-12]
            if shortname.endswith(': white hair'):
                shortname = "white_haried_" + shortname[:-12]
            if shortname.endswith(': bald'):
                shortname = "bald_" + shortname[:-6]
            if shortname.endswith(': beard'):
                shortname = "bearded_" + shortname[:-7]
            if shortname.endswith(' face'):
                shortname = shortname[:-5]
            if shortname.endswith(' button'):
                shortname = shortname[:-7]
            if shortname.endswith(' banknote'):
                shortname = shortname[:-9]

            # FIXME: Is there a better way to do this?
            matchobj = re.match(r'^flag: (.*)$', shortname)
            if shortname.startswith("flag: "):
                country = shortname[5:]
                shortname = country + " flag"
            shortname = shortname.replace("u.s.", "us")
            shortname = shortname.replace("&", "and")

            if shortname == name.lower():
                basicallyTheSame = True

            shortname = shortname.replace("-", "_")
            shortname = re.sub(r'\W', '_', shortname)
            shortname, = re.match(r'^_*(.+)_*$', shortname).groups()
            shortname = re.sub(r'_{2,}', '_', shortname)
            shortname = unidecode(shortname)
        # if basicallyTheSame: 
        #    shortname = ""
        categories[current_category].append(Emoji(code, shortname, name))

    # Use xclip to pipe the output to clipboard.
    # e.g ./emoji_codegen.py emoji.json | xclip -sel clip
    # alternatively - delete the var from src/emoji/Provider.cpp, and do ./codegen.sh emojis shortcodes >> ../src/emoji/Provider.cpp
    generate_qml_list(people=people, nature=nature, food=food, activity=activity, travel=travel, objects=objects, symbols=symbols, flags=flags)
