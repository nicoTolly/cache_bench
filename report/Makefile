ROOT=.
BIN_DIR=build
BIN=$(BIN_DIR)/report.pdf
SRC=$(ROOT)/report.tex

.PHONY: all

all: $(BIN)

$(BIN): $(SRC)
	pdflatex -output-directory=$(BIN_DIR) $< 




