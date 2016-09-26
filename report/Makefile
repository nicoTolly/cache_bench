ROOT=.
BIN_DIR=build
BIN=$(BIN_DIR)/report.pdf
SRC=$(ROOT)/report.tex $(ROOT)/report.bib

.PHONY: all
.PHONY: mrproper

all: $(BIN)

$(BIN): $(SRC)
	pdflatex -output-directory=$(BIN_DIR) $< && bibtex $(BIN_DIR)/report && pdflatex -output-directory=$(BIN_DIR) $< 


$(ROOT)/report.bib:
	bibtex $(BIN_DIR)/$< 


mrproper:
	rm -f $(BIN_DIR)/*
