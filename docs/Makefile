# $Id: Makefile.gcc,v 1.17 1999/11/22 03:19:24 kaz Exp $
# $Name: kazlib_1_20 $

kazlib: kazlib.pdf

kazlib.pdf: kazlib.dvi
	dvipdf kazlib.dvi

kazlib.dvi: kazlib.ltx kazlib.toc kazlib.ind
	latex kazlib.ltx

kazlib.ind: kazlib.idx kazlib.ist
	makeindex -s kazlib.ist kazlib.idx

kazlib.toc: kazlib.ltx
	latex kazlib.ltx

kazlib.idx: kazlib.ltx
	latex kazlib.ltx

clean:
	-rm kazlib.aux kazlib.log kazlib.dvi kazlib.toc kazlib.idx kazlib.ind kazlib.ilg

