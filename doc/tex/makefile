TEX =	compiling.tex configuration.tex faq.tex fileselector.tex foreword.tex \
	install.tex mdz.tex opencp.tex player.tex plugin.tex starting.tex \
	support.tex thanks.tex wap.tex

TEXTEMP=	h

L2H+=	-address "documentation by <a href=\"mailto:doj@cubic.org\">doj</a> / <a href=\"http://www.cubic.org\">cubic</a>"
L2H+=	-local_icons -transparent -white -no_footnode -antialias -antialias_text -short_extn
L2H+=	-up_url "http://www.cubic.org/player" -up_title "OpenCP homepage"

export TEXINPUTS=:./inputs

all:		$(TEX)
		touch opencp.tex
		$(MAKE) opencp.dvi

html:		
		perl prepare < opencp.tex > $(TEXTEMP).tex
		$(MAKE) $(TEXTEMP).dvi
		touch $(TEXTEMP).tex
		$(MAKE) $(TEXTEMP).dvi
		touch $(TEXTEMP).tex
		$(MAKE) $(TEXTEMP).dvi
		latex2html $(L2H) $(TEXTEMP)
		rm -f $(TEXTEMP)/*.pl
		rm -f $(TEXTEMP)/*.aux
		rm -f $(TEXTEMP)/*.log
		rm -f $(TEXTEMP)/*.tex
		rm -f $(TEXTEMP)/warnings

ps:		opencp.ps

pdf:		opencp.pdf

clean:	
		rm -f *.aux *.toc *.log *.dvi *~ *.ps *.pdf *.zip *.bz2 $(TEXTEMP).tex
		rm -rf $(TEXTEMP)

%.ps:		%.dvi
		dvips -o $@ $<

%.pdf:		%.dvi
		dvipdf $< $@

%.dvi:		%.tex
		if [ -f $@ ]; \
		then latex $< ; \
		else latex $< ; \
		     latex $< ; \
		     latex $< ; \
		fi
