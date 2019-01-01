import re, os, sys, subprocess

"""
Fixes cases such as <h1 id="problematic">...</h1>
Problematic IDs are those that are duplicate or identical with the headline content,
e.g. <h1 id="menu">menu</h1>, see https://groups.google.com/d/msg/pandoc-discuss/7DAy496S54I/peLZeKwzCgAJ
"""

def iter_all(suffix):
    for root, dirs, files in os.walk("."):
        # print(root, dirs, files)
        for fn in files:
            if fn.endswith(suffix):
                p = os.path.join(root, fn)
                if len(sys.argv) < 2 or sys.argv[1] in p:
                    # print(p)
                    yield p

        for excl in [".git", "_build", "_static", "_templates"]:
            if excl in dirs:
                dirs.remove(excl)

allIDs = {}

# Find all IDs.
for p in iter_all(".html"):
    with open(p, encoding='utf-8') as f:
        html = f.read()
        for tp in re.findall(r'<.+?id="(.*?)".*?>(.*?)<', html):
            ID = tp[0]
            if ID not in allIDs:
                allIDs[ID] = [p]
            if p not in allIDs[ID]:
                allIDs[ID].append(p)
                if len(allIDs[ID]) > 1:
                    print("dup: ", ID, allIDs[ID])
            if ID.lower() == tp[1].lower():
                print("fix id == content:", ID, tp[1])
                allIDs[ID].append("force rewrite")

print("\n---- len allIDs", len(allIDs), "----\n")
curr_path = None

def rewrite_id(matchobj):
    global curr_path, allIDs
    s = matchobj.group(0)   # the entire match
    id_ = matchobj.group(1)
    assert id_ in allIDs
    l = allIDs[id_]
    if len(l) <= 1:
        return s
    xp = curr_path.replace(".html", "").replace(".\\", "").replace("\\", "_").replace(".", "")
    neu = 'id="{}_{}"'.format(xp, id_)
    print("ersetze", s, "durch", neu)
    return neu

# Rewrite all id="dup".
for p in iter_all(".html"):
    curr_path = p
    with open(p, encoding='utf-8') as f:
        html = f.read()
        html = re.sub(r'id="(.*?)"', rewrite_id, html)
    # with open(p, encoding='utf-8', mode='w') as f:
    #     f.write(html)

def expand_anchor(matchobj):
    global allIDs
    s = matchobj.group(0)   # the entire match
    if s.startswith('href="/_detail/') or s.startswith('href="/_media/') or s.startswith('href="/lib/exe/'):
        return s
    page, raute, id_ = matchobj.group(1).partition("#")
    #print(page, raute, id_)
    assert id_ and id_ in allIDs
    l = allIDs[id_]
    if len(l) <= 1:
        return s
    neu = 'href="/' + page + "#" + page.replace(":", "_") + "_" + id_ + '"'
    print("ersetze", s, "durch", neu)
    return neu

# Rewrite all href="...#dup".
for p in iter_all(".html"):
    with open(p, encoding='utf-8') as f:
        html = f.read()
        html = re.sub(r'href="/(.*?)"', expand_anchor, html)
    # with open(p, encoding='utf-8', mode='w') as f:
    #     f.write(html)
