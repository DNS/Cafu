import os, sys, subprocess


for root, dirs, files in os.walk("."):
    # print(root, dirs, files)
    for fn in files:
        if fn.endswith(".html"):
            p = os.path.join(root, fn)
          # if "modeleditor" in p and "scenesetup" in p:      # for testing
            if True:
                print(p)
                subprocess.Popen(["pandoc", p, "--lua-filter", "dw_filter.lua", "-o", p.replace(".html", ".rst")]).wait()

    for excl in [".git", "_build", "_static", "_templates"]:
        if excl in dirs:
            dirs.remove(excl)
