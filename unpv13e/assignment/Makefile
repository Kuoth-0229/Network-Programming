include ../Make.defines
 
PROGS = ass1cli24 ass2cli24  ass3cli24  ass5cli24 ass5serv24 ass6cli24 ass6serv24
all:	${PROGS} 

ass1cli24:	ass1cli24.o
		${CC} ${CFLAGS} -o $@ ass1cli24.o ${LIBS}

ass2cli24:	ass2cli24.o
		${CC} ${CFLAGS} -o $@ ass2cli24.o ${LIBS}

ass3cli24:	ass3cli24.o
		${CC} ${CFLAGS} -o $@ ass3cli24.o ${LIBS}

ass5cli24:	ass5cli24.o
		${CC} ${CFLAGS} -o $@ ass5cli24.o ${LIBS}

ass5serv24:	ass5serv24.o
		${CC} ${CFLAGS} -o $@ ass5serv24.o ${LIBS}		

ass6serv24:	ass6serv24.o
		${CC} ${CFLAGS} -o $@ ass6serv24.o ${LIBS}

ass6cli24:	ass6cli24.o
		${CC} ${CFLAGS} -o $@ ass6cli24.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
