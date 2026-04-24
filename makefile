# =============================================================================
# Makefile — ted (Estrutura de Dados / UEL)
# =============================================================================

PROJ_NAME = ted
ALUNO     = Ana Vitória Oliver Machado

CC      = gcc
CFLAGS  = -ggdb -O0 -std=c99 -fstack-protector-all \
          -Werror=implicit-function-declaration \
          -DUNITY_INCLUDE_DOUBLE
LDFLAGS = -lm

INC = -I include -I .

# =============================================================================
# Objetos dos módulos (sem main)
# =============================================================================

MODS = src/hash.o    \
       src/args.o    \
       src/svg.o     \
       src/cidade.o  \
       src/pessoas.o \
       src/qry.o

UNITY_OBJ = unity/src/unity.o

# =============================================================================
# Target principal
# =============================================================================

.PHONY: all ted clean tstall \
        tst_hash tst_args tst_svg tst_cidade tst_pessoas tst_qry

all: ted

ted: $(MODS) src/main.o
	$(CC) -o src/$(PROJ_NAME) $^ $(LDFLAGS)
	@echo "Executavel 'src/$(PROJ_NAME)' criado."

# =============================================================================
# Dependências explícitas dos módulos
# =============================================================================

src/hash.o: src/hash.c include/hash.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/args.o: src/args.c include/args.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/svg.o: src/svg.c include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/cidade.o: src/cidade.c include/cidade.h include/hash.h include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/pessoas.o: src/pessoas.c include/pessoas.h include/hash.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/qry.o: src/qry.c include/qry.h \
           include/cidade.h include/pessoas.h include/hash.h include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

src/main.o: src/main.c \
            include/args.h include/hash.h include/svg.h \
            include/cidade.h include/pessoas.h include/qry.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

unity/src/unity.o: unity/src/unity.c unity/src/unity.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# =============================================================================
# Dependências explícitas dos testes
# =============================================================================

test/t_hash.o: test/t_hash.c include/hash.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test/t_args.o: test/t_args.c include/args.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test/t_svg.o: test/t_svg.c include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test/t_cidade.o: test/t_cidade.c include/cidade.h include/hash.h include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test/t_pessoas.o: test/t_pessoas.c include/pessoas.h include/hash.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test/t_qry.o: test/t_qry.c include/qry.h \
              include/cidade.h include/pessoas.h include/hash.h include/svg.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# =============================================================================
# Targets de teste — cada um compila e executa
# =============================================================================

tst_hash: src/hash.o test/t_hash.o $(UNITY_OBJ)
	$(CC) -o test/run_hash $^ $(LDFLAGS)
	./test/run_hash

tst_args: $(MODS) test/t_args.o $(UNITY_OBJ)
	$(CC) -o test/run_args $^ $(LDFLAGS)
	./test/run_args

tst_svg: src/svg.o test/t_svg.o $(UNITY_OBJ)
	$(CC) -o test/run_svg $^ $(LDFLAGS)
	./test/run_svg

tst_cidade: src/hash.o src/svg.o src/cidade.o test/t_cidade.o $(UNITY_OBJ)
	$(CC) -o test/run_cidade $^ $(LDFLAGS)
	./test/run_cidade

tst_pessoas: src/hash.o src/pessoas.o test/t_pessoas.o $(UNITY_OBJ)
	$(CC) -o test/run_pessoas $^ $(LDFLAGS)
	./test/run_pessoas

tst_qry: $(MODS) test/t_qry.o $(UNITY_OBJ)
	$(CC) -o test/run_qry $^ $(LDFLAGS)
	./test/run_qry

tstall: tst_hash tst_args tst_svg tst_cidade tst_pessoas tst_qry

# =============================================================================
# Limpeza
# =============================================================================

clean:
	find . -name '*.o' -delete
	rm -f src/$(PROJ_NAME) test/run_*
	@echo "Limpeza concluida."