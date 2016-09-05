ROOT=.
BIN_DIR=build
BIN=$(BIN_DIR)/report.pdf
SRC=$(ROOT)/report.tex

.PHONY: all
.PHONY: mrproper

all: $(BIN)

$(BIN): $(SRC)
	pdflatex -output-directory=$(BIN_DIR) $< 



mrproper:
	rm -f $(BIN_DIR)/*
