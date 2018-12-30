import re, os, sys, subprocess

"""
For one-time use, this script removes all id="..." attributes that are unused,
e.g. in  <h1 id="unreferenced">...</h1>
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

allAnchors = set()

# Find all referred anchors/ids.
for p in iter_all(".html"):
    with open(p, encoding='utf-8') as f:
        html = f.read()
        for anchor in re.findall(r'href="/.*?#([^"]*?)"', html):
            if anchor not in allAnchors:
                print(anchor)
            allAnchors.add(anchor)

KeptCount = 0
RemvCount = 0

def remove_anchor(matchobj):
    global KeptCount, RemvCount
    s = matchobj.group(0)   # the entire match
    id_ = matchobj.group(1)
    if id_ in allAnchors:
        print("kept:", id_)
        KeptCount += 1
        return s
    else:
        print("REMV:", id_)
        RemvCount += 1
        return ""

# Remove all unused anchors/ids.
for p in iter_all(".html"):
    with open(p, encoding='utf-8') as f:
        html = f.read()
        html = re.sub(r' id="(.*?)"', remove_anchor, html)

    # with open(p, encoding='utf-8', mode='w') as f:
    #     f.write(html)

print("kept", KeptCount, "removed", RemvCount)
