from dokuwiki import DokuWiki
import os, sys

# https://pypi.org/project/dokuwiki/
dw = DokuWiki("https://docs.cafu.de", "Carsten", sys.argv[1])

for root, dirs, files in os.walk("."):
    # print(root, dirs, files)
    for fn in files:
        if fn.endswith(".txt"):
            dw_name = os.path.join(root, fn)[2:].replace(".txt", "").replace("\\", ":")
            s = dw.pages.html(dw_name)

            out_name = os.path.join(root, fn).replace(".txt", ".html")
            print(out_name)
            with open(out_name, encoding='utf-8', mode='w') as f:
                f.write(s)

    for excl in [".git", "_build", "_static", "_templates"]:
        if excl in dirs:
            dirs.remove(excl)
