all: env pipetest

env:
ifndef XV6_PATH
	$(error XV6_PATH not set)
endif

pipetest: pipetest.c
	gcc  -I$(XV6_PATH) -o pipetest pipetest.c

.PHONY: clean all

clean:
	rm pipetest
