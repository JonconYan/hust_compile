parser: lex.l parser.y ast.c def.h analysis.c
	bison -d -v parser.y
	flex lex.l
	cc -o parser lex.yy.c parser.tab.c ast.c analysis.c

clean:
	-rm -rf lex.yy.c
	-rm -rf parser.tab.h
	-rm -rf parser.tab.c
	-rm -rf parser.output
	-rm -rf parser