import re, os, sys, subprocess

"""
For one-time use, this script fixes in our HTML source files the <a href="fixme"> attributes:
It fixes all internal hyperlinks that don't refer to an explicit anchor by adding one.
Examples:
  - good as is:
    <a href="/general:developer_faq#how_do_i_dynamically_reload_the_map_script_in-game" class="wikilink1">...</a>

  - <a href="/modeleditor:mainwindow" class="wikilink1" title="modeleditor:mainwindow">The Main Window</a>
    we open file /modeleditor/mainwindow.html, look up the id="..." of the first headline (<h1 id="some_id">)
    and insert it into the above href:
    <a href="/modeleditor:mainwindow#some_id" class="wikilink1" title="modeleditor:mainwindow">The Main Window</a>
"""

def insert_anchor(matchobj):
    s = matchobj.group(0)   # the entire match
    if s.startswith('href="/_detail/') or s.startswith('href="/_media/') or s.startswith('href="/lib/exe/'):
        return s
    if "#" in s:
        return s

    fn = matchobj.group(1).replace(":", "/") + ".html"
    print("    ", s)
    try:
        with open(fn, encoding='utf-8') as f:
            html = f.read()
    except FileNotFoundError:
        print("#### not found ", fn)
        assert False
        return s

    m = re.search(r'<h[12].*?id="(.*?)".*?>', html)
    s = s[0:-1] + "#" + m.group(1) + '"'
    print("  ->", s)

    return s


for root, dirs, files in os.walk("."):
    # print(root, dirs, files)
    for fn in files:
        if fn.endswith(".html"):
            p = os.path.join(root, fn)
            if len(sys.argv) < 2 or sys.argv[1] in p:
                print(p)

                # Search file `p` for all <a href="..."> that don't refer to an anchor.
                with open(p, encoding='utf-8') as f:
                    html = f.read()
                    html = re.sub(r'href="/(.*?)"', insert_anchor, html)

                with open(p, encoding='utf-8', mode='w') as f:
                    f.write(html)

    for excl in [".git", "_build", "_static", "_templates"]:
        if excl in dirs:
            dirs.remove(excl)
