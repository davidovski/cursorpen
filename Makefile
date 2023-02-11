CC		?= gcc
PROG	= cursorpen
FLAGS	= -lX11 -lXtst -lXi -lpthread -ludev -levdev

${PROG}: ${PROG}.o 
	${CC} ${PROG}.c -o ${PROG} ${FLAGS}
