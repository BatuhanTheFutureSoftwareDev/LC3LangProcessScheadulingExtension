C = gcc 
CFLAGS = -std=c11 -Wall

MAIN = main.c
VM = vm

LC3L = lc3l
MPC = mpc.c

BIN_PROGRAM1 = bin_programs/simple
BIN_PROGRAM2 = bin_programs/brk
BIN_PROGRAM3 = bin_programs/brk2
BIN_PROGRAM4 = bin_programs/yld

LANG_PROGRAM1 = lang_programs/test1
LANG_PROGRAM2 = lang_programs/test2
LANG_PROGRAM3 = lang_programs/test3
LANG_PROGRAM4 = lang_programs/test4
LANG_PROGRAM5 = lang_programs/test5
LANG_PROGRAM6 = lang_programs/test6
LANG_PROGRAM7 = lang_programs/test7
LANG_PROGRAM8 = lang_programs/test8
LANG_PROGRAM9 = lang_programs/test9
LANG_PROGRAM10 = lang_programs/test10
LANG_PROGRAM11 = lang_programs/test11
LANG_PROGRAM12 = lang_programs/test12
LANG_PROGRAM13 = lang_programs/test13

BIN_OBJ1 = bin_programs/simple_code.obj bin_programs/simple_heap.obj
BIN_OBJ2 = bin_programs/brk_code.obj bin_programs/brk_heap.obj
BIN_OBJ3 = bin_programs/brk2_code.obj bin_programs/brk2_heap.obj
BIN_OBJ4 = bin_programs/yld_code.obj bin_programs/yld_heap.obj

LANG_OBJ1 = lang_programs/test1_code.obj lang_programs/test1_heap.obj
LANG_OBJ2 = lang_programs/test2_code.obj lang_programs/test2_heap.obj
LANG_OBJ3 = lang_programs/test3_code.obj lang_programs/test3_heap.obj
LANG_OBJ4 = lang_programs/test4_code.obj lang_programs/test4_heap.obj
LANG_OBJ5 = lang_programs/test5_code.obj lang_programs/test5_heap.obj
LANG_OBJ6 = lang_programs/test6_code.obj lang_programs/test6_heap.obj
LANG_OBJ7 = lang_programs/test7_code.obj lang_programs/test7_heap.obj
LANG_OBJ8 = lang_programs/test8_code.obj lang_programs/test8_heap.obj
LANG_OBJ9 = lang_programs/test9_code.obj lang_programs/test9_heap.obj
LANG_OBJ10 = lang_programs/test10_code.obj lang_programs/test10_heap.obj
LANG_OBJ11 = lang_programs/test11_code.obj lang_programs/test11_heap.obj
LANG_OBJ12 = lang_programs/test12_code.obj lang_programs/test12_heap.obj
LANG_OBJ13 = lang_programs/test13_code.obj lang_programs/test13_heap.obj

TEST1 = tests/initos-test
TEST2 = tests/mem-test
TEST3 = tests/mem-test2
TEST4 = tests/proc-test
TEST5 = tests/mw-mr-test
TEST6 = tests/mw-mr-test2

.PHONY: all clean sample tests bin_programs lang_programs

all: clean sample tests bin_programs lang_programs

bin_programs: $(BIN_PROGRAM1).c $(BIN_PROGRAM2).c $(BIN_PROGRAM3).c $(BIN_PROGRAM4).c
	@$(C) $(CFLAGS) $(BIN_PROGRAM1).c -o $(BIN_PROGRAM1)
	@$(C) $(CFLAGS) $(BIN_PROGRAM2).c -o $(BIN_PROGRAM2)
	@$(C) $(CFLAGS) $(BIN_PROGRAM3).c -o $(BIN_PROGRAM3)
	@$(C) $(CFLAGS) $(BIN_PROGRAM4).c -o $(BIN_PROGRAM4)

	@$(BIN_PROGRAM1)
	@$(BIN_PROGRAM2)
	@$(BIN_PROGRAM3)
	@$(BIN_PROGRAM4)

	@rm $(BIN_PROGRAM1) $(BIN_PROGRAM2) $(BIN_PROGRAM3) $(BIN_PROGRAM4)

lang_programs: $(LANG_PROGRAM1).lc3 $(LANG_PROGRAM2).lc3 $(LANG_PROGRAM3).lc3 $(LANG_PROGRAM4).lc3 $(LANG_PROGRAM5).lc3 $(LANG_PROGRAM6).lc3 $(LANG_PROGRAM7).lc3 $(LANG_PROGRAM8).lc3 $(LANG_PROGRAM9).lc3 $(LANG_PROGRAM10).lc3 $(LANG_PROGRAM11).lc3 $(LANG_PROGRAM12).lc3 $(LANG_PROGRAM13).lc3
	@./$(LC3L) $(LANG_PROGRAM1).lc3
	@./$(LC3L) $(LANG_PROGRAM2).lc3
	@./$(LC3L) $(LANG_PROGRAM3).lc3
	@./$(LC3L) $(LANG_PROGRAM4).lc3
	@./$(LC3L) $(LANG_PROGRAM5).lc3
	@./$(LC3L) $(LANG_PROGRAM6).lc3
	@./$(LC3L) $(LANG_PROGRAM7).lc3
	@./$(LC3L) $(LANG_PROGRAM8).lc3
	@./$(LC3L) $(LANG_PROGRAM9).lc3
	@./$(LC3L) $(LANG_PROGRAM10).lc3
	@./$(LC3L) $(LANG_PROGRAM11).lc3
	@./$(LC3L) $(LANG_PROGRAM12).lc3
	@./$(LC3L) $(LANG_PROGRAM13).lc3

tests: $(TEST1).c $(TEST2).c $(TEST3).c $(TEST4).c $(TEST5).c $(TEST6).c
	@$(C) $(CFLAGS) $(TEST1).c -o $(TEST1)
	@$(C) $(CFLAGS) $(TEST2).c -o $(TEST2)
	@$(C) $(CFLAGS) $(TEST3).c -o $(TEST3)
	@$(C) $(CFLAGS) $(TEST4).c -o $(TEST4)
	@$(C) $(CFLAGS) $(TEST5).c -o $(TEST5)
	@$(C) $(CFLAGS) $(TEST6).c -o $(TEST6)

sample: $(MAIN)
	@$(C) $(CFLAGS) $(MAIN) -o $(VM)
	@$(C) $(CFLAGS) $(LC3L).c $(MPC) -o $(LC3L)

clean:
	@rm -f $(BIN_OBJ1) $(BIN_OBJ2) $(BIN_OBJ3) $(BIN_OBJ4) $(LANG_OBJ1) $(LANG_OBJ2) $(LANG_OBJ3) $(LANG_OBJ4) $(LANG_OBJ5) $(LANG_OBJ6) $(LANG_OBJ7) $(LANG_OBJ8) $(LANG_OBJ9) $(LANG_OBJ10) $(LANG_OBJ11) $(LANG_OBJ12) $(LANG_OBJ13) $(LANG_PROGRAM1).asm $(LANG_PROGRAM2).asm $(LANG_PROGRAM3).asm $(LANG_PROGRAM4).asm $(LANG_PROGRAM5).asm $(LANG_PROGRAM6).asm $(LANG_PROGRAM7).asm $(LANG_PROGRAM8).asm $(LANG_PROGRAM9).asm $(LANG_PROGRAM10).asm $(LANG_PROGRAM11).asm $(LANG_PROGRAM12).asm $(LANG_PROGRAM13).asm $(TEST1) $(TEST2) $(TEST3) $(TEST4) $(TEST5) $(TEST6) $(VM) $(LC3L)