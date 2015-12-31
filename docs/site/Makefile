WEBSITE=~/mongrel2.org/site-root

all:
	rsync -a --delete assets/ output/
	mkdir -p output/manual
	lua gensite.lua output
	bash -c 'cp -a ../../output/docs/manual/*.{html,css} output/manual'

clean:
	rm -rf output

sync: all
	rsync -avz output/* $(WEBSITE)/

