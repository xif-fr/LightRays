CPPFLAGS := -O3 -Wall -DSFMLC01_WINDOW_UNIT=720 -Dvec2_t=vec_t -Dpt2_t=point_t -DFONT_PATH=\"DejaVuSansMono.ttf\"
LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system

all: brouillard diffus_test milieux store

COMMON := Rayon.o Util.o Scene.o SceneTest.o Ecran.o ObjetsCourbes.o Source.o ObjetsOptiques.o

brouillard: $(COMMON) Brouillard.o ObjetDiffusant.o main_brouillard.o
	g++ -o lightrays-brouillard -lm $^ $(LDFLAGS)
	./lightrays-brouillard

diffus_test: $(COMMON) ObjetDiffusant.o main_diffus_test.o
	g++ -o lightrays-diffus_test -lm $^ $(LDFLAGS)
	./lightrays-diffus_test

milieux: $(COMMON) ObjetMilieux.o main_milieux.o
	g++ -o lightrays-milieux -lm $^ $(LDFLAGS)
	./lightrays-milieux

store: $(COMMON) ObjetDiffusant.o main_store.o
	g++ -o lightrays-store -lm $^ $(LDFLAGS)
	./lightrays-store

%.o: %.cpp
	g++ -o $@ -c $< -std=c++17 $(CPPFLAGS)

clean:
	rm *.o
	rm lightrays-*
