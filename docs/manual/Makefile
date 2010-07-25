JUNK_FILES=book-final.* *.aux *.log styles/*.aux
SOURCE=book
WEBSITE=zedshaw.com:/usr/local/nginx/html/projects/build_your_own_mongrel

draft: book-final.dvi
	htlatex book-final.tex
	sed -ei 's/\[/\&#91;/g' book-final.html
	sed -ei 's/\]/\&#93;/g' book-final.html

book-final.dvi:
	cp $(SOURCE).tex book-final.tex
	latex -halt-on-error book-final.tex

book-final.pdf: book-final.dvi
	dvipdf book-final.dvi

view: book-final.pdf
	evince book-final.pdf

clean:
	rm -rf $(JUNK_FILES)
	find . -name "*.aux" -exec rm {} \;

release: clean book-final.pdf
	rsync -vz $(FINAL).pdf $(WEBSITE)


