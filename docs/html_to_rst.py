import os, sys, subprocess


for root, dirs, files in os.walk("."):
    # print(root, dirs, files)
    for fn in files:
        if fn.endswith(".html"):
            p = os.path.join(root, fn)
            if "modeleditor" in p:      # for testing
                print(p)
                subprocess.Popen(["pandoc", p, "--lua-filter", "dw_filter.lua", "-o", p.replace(".html", ".rst")]).wait()
