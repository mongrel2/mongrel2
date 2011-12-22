WEBSITE=$(USER)@mongrel2.org:/var/www/mongrel2.org/

all:
	rsync -a --delete assets/ output/
	lua gensite.lua output

clean:
	rm -rf output

sync: all
	rsync -avz output/* $(WEBSITE)/

