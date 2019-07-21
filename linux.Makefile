.PHONY: all
all: build

.PHONY: prepare
prepare:
	sudo apt install build-essential make zlib1g zlib1g-dev

src/pkg/linux/links/runner.jar:
	mkdir -p src/pkg/linux/links
	./gradlew fatJar
	cp build/libs/TechnoRunner-1.0-SNAPSHOT.jar src/pkg/linux/links/runner.jar

src/pkg/linux/links/jre.tar.gz:
	mkdir -p src/pkg/linux/links
	wget https://minecraft.glitchless.ru/jres/jre-8u202-linux-x64.tar.gz -O src/pkg/linux/links/jre.tar.gz

src/pkg/linux/links/jrepath.txt:
	mkdir -p src/pkg/linux/links
	echo 'jre1.8.0_202/bin/java' > src/pkg/linux/links/jrepath.txt

.PHONY: build
build: src/pkg/linux/links/runner.jar src/pkg/linux/links/jre.tar.gz src/pkg/linux/links/jrepath.txt
	cd src/pkg && gcc -static -I. linux/pkg.c linux/pkg.s pkg_generic.c util.c -lz -o ../../build/pkg-linux

.PHONY: clean
clean:
	rm -f src/pkg/linux/links/runner.jar src/pkg/linux/links/jre.tar.gz src/pkg/linux/links/jrepath.txt build/pkg-linux

