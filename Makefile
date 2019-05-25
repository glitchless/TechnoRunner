.PHONY: dir-build runner-linux runner-macos runner-windows jre-linux jre-macos jre-windows pkg-linux pkg-macos pkg-linux


# dirs
dir-build:
	mkdir -p build


# runner
runner-linux: dir-build
	./gradlew build

runner-macos: dir-build
	./gradlew build

runner-windows: dir-build
	./gradlew.bat build


# jre
build/jre-linux.tar.gz: dir-build
	wget https://minecraft.glitchless.ru/jres/jre-8u202-linux-x64.tar.gz -O build/jre-linux.tar.gz

build/jrepath-linux.txt: dir-build
	echo 'jre1.8.0_202/bin/java' > build/jrepath-linux.txt

jre-linux: build/jre-linux.tar.gz build/jrepath-linux.txt

build/jre-macos.tar.gz: dir-build
	wget https://minecraft.glitchless.ru/jres/jre-8u202-macosx-x64.tar.gz -O build/jre-macos.tar.gz

build/jrepath-macos.txt: dir-build
	echo 'jre1.8.0_202/bin/java' > build/jrepath-macos.txt

jre-macos: build/jre-macos.tar.gz build/jrepath-macos.txt

build/jre-windows.tar.gz: dir-build
	wget https://minecraft.glitchless.ru/jres/jre-8u202-windows-x64.tar.gz -O build/jre-windows.tar.gz

build/jrepath-windows.txt: dir-build
	echo 'jre1.8.0_202/bin/java.txt' > build/jrepath-windows.txt

jre-windows: build/jre-windows.tar.gz build/jrepath-windows.txt


# pkg
pkg-linux: #runner-linux jre-linux
	mkdir -p src/pkg/linux/links
	cp build/jre-linux.tar.gz src/pkg/linux/links/jre.tar.gz
	cp build/jrepath-linux.txt src/pkg/linux/links/jrepath.txt
	cp build/libs/TechnoRunner-1.0-SNAPSHOT.jar src/pkg/linux/links/runner.jar
	cd src/pkg && gcc -g -static -I. linux/pkg.c linux/pkg.s pkg_generic.c util.c -lz -o ../../build/pkg-linux

pkg-macos: runner-macos jre-macos
	mkdir -p src/pkg/macos/links
	cp build/jre-macos.tar.gz src/pkg/macos/links/jre.tar.gz
	cp build/jrepath-macos.txt src/pkg/macos/links/jrepath.txt
	cp build/libs/TechnoRunner-1.0-SNAPSHOT.jar src/pkg/macos/links/runner.jar
	cd src/pkg
	gcc macos/pkg.c macos/pkg.s pkg_generic.c ungz.c untar.c util.c -o ../../build/pkg-macos
	cd ../..

pkg-windows: runner-windows jre-windows
	echo 'not implemented'
