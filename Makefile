PROJECT = typeset
OUT = $(PROJECT)
FLAGS = -D_GNU_SOURCE

all: $(OUT)

run: $(OUT)
	@./$(OUT)

test: $(OUT)
	@./$(OUT) ./test.txt

clean:
	rm -f $(OUT)

$(OUT): $(PROJECT).c
	gcc $(PROJECT).c -o $(OUT) $(FLAGS) -lm

.PHONY: all clean
