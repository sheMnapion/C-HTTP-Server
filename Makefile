# Do *NOT* modify the existing build rules.
# You may add your own rules, e.g., "make run" or "make test".

LAB = httpd
PORT ?= 8000
FOLDER ?= ./site
ARGS ?= --port $(PORT) $(FOLDER)

include Makefile.git

.PHONY: build submit run clean

build: $(LAB).c
	$(call git_commit, "compile")
	gcc -std=gnu99 -O1 -Wall -o $(LAB) $(LAB).c -lpthread

run: build
	@./$(LAB) $(ARGS)

anotherRun: build
	@./$(LAB) -p 8080 ../../http-server-161220062/2017/http-server

clean:
	rm -f $(LAB)

submit:
	cd .. && tar cj $(LAB) > submission.tar.bz2
	curl -F "task=M7" -F "id=$(STUID)" -F "name=$(STUNAME)" -F "submission=@../submission.tar.bz2" 114.212.81.90:5000/upload
