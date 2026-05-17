EPSPDF=epstopdf
LATEX=pdflatex

%.pdf: %.msc
	mscgen -T eps -o $@ -i $<
	$(EPSPDF) $@

all: subscribe.pdf update.pdf
	$(LATEX) spec.tex
