.PHONY: all
all: build


src/pkg/windows/links/runner.jar:
	mkdir -p src/pkg/windows/links
	./gradlew fatJar
	cp build/libs/TechnoRunner-1.0-SNAPSHOT.jar src/pkg/windows/links/runner.jar

src/pkg/windows/links/jre.tar.gz:
	mkdir -p src/pkg/windows/links
	wget https://minecraft.glitchless.ru/jres/jre-8u202-windows-x64.tar.gz -O src/pkg/windows/links/jre.tar.gz

src/pkg/windows/links/jrepath.txt:
	mkdir -p src/pkg/windows/links
	echo 'jre1.8.0_202/bin/java.exe' > src/pkg/windows/links/jrepath.txt

.PHONY: build
build: src/pkg/windows/links/runner.jar src/pkg/windows/links/jre.tar.gz src/pkg/windows/links/jrepath.txt
	cd src/pkg && gcc -g -I. windows/pkg.c windows/pkg.s pkg_generic.c util.c -lz -o ../../build/pkg-windows

.PHONY: clean
clean:
	rm -f src/pkg/windows/links/runner.jar src/pkg/windows/links/jre.tar.gz src/pkg/windows/links/jrepath.txt build/pkg-windows

