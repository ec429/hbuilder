#!/usr/bin/python
# encoding: utf-8
import random

techs = {}

with open('tech', 'r') as f:
    for line in f.readlines():
        words = line.split(':')
        ident = words.pop(0)
        reqs = set()
        year = None
        inter = False
        name = None
        engines = []
        turrets = []
        data = {}
        for word in words:
            k, _, v = word.partition('=')
            if k == 'r':
                assert v in techs
                reqs.add(v)
            elif k == 'y':
                year = int(v)
            elif k == 'i':
                inter = bool(int(v))
            elif k == 'n':
                name = v
            elif k == 'e':
                engines.append(v)
            elif k == 't':
                turrets.append(v)
            else:
                data[k] = int(v)
        data['engines'] = engines
        data['turrets'] = turrets
        data['year'] = year
        data['inter'] = inter
        data['name'] = name
        data['reqs'] = reqs
        techs[ident] = data

have = set()
def check_reqs(data):
    reqs = data['reqs']
    for r in reqs:
        if r not in have:
            return False
    return True
for k,v in techs.items():
    if not v['year']:
        have.add(k)
        print "Start:", v['name']
for k,v in techs.items():
    if k not in have and v['year'] < 1939:
        have.add(k)
        print "Start2:", v['name']
m = 9
y = 1939
while y < 1945 or m < 5:
    m += 1
    if m > 12:
        y += 1
        m -= 12
    avail = set(k for k,v in techs.items() if k not in have and check_reqs(v))
    ayear = set(t for t in avail if techs[t]['year'] <= y)
    if not ayear:
        if m < 7 or random.random() < 0.7:
            continue
        ayear = set(t for t in avail if techs[t]['year'] <= y + 1)
    elif random.random() < 0.3:
        continue
    if not ayear:
        continue
    choose = random.choice(list(ayear))
    chosen = techs[choose]
    print "%2d-%4d:" % (m, y), chosen['name'], '(%d)' % chosen['year']
    have.add(choose)

print "Unresearched:", ', '.join('%s (%d)' % (v['name'], v['year']) for k,v in techs.items() if k not in have)
