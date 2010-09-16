PDF=mongrel2-1.0-guide-ru.pdf
JUNK_FILES=book-final.* *.aux *.log styles/*.aux book-final* $(PDF)
SOURCE=book
WEBSITE=zedshaw.com:/usr/local/nginx/html/projects/build_your_own_mongrel

draft: book-final.dvi
	htlatex book-final.tex
	tidy -quiet -ashtml -omit -ic -m book-final.html
	sed -f clean.sed book-final.html > book.wiki

book-final.dvi:
	cp $(SOURCE).tex book-final.tex
	latex -halt-on-error book-final.tex

book-final.pdf: book-final.dvi
	dvipdf book-final.dvi $(PDF)

view: book-final.pdf
	evince book-final.pdf

clean:
	rm -rf $(JUNK_FILES)
	find . -name "*.aux" -exec rm {} \;

release: clean book-final.pdf
	rsync -vz $(FINAL).pdf $(WEBSITE)

pdf: book-final.pdf
