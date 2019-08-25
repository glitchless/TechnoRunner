.PHONY: all
all: build-app

.PHONY: prepare
prepare:
	brew install gcc
	brew install zlib

src/pkg/macos/links/runner.jar:
	mkdir -p src/pkg/macos/links
	./gradlew fatJar
	cp build/libs/TechnoRunner-1.0-SNAPSHOT.jar src/pkg/macos/links/runner.jar

src/pkg/macos/links/jre.tar.gz:
	mkdir -p src/pkg/macos/links
	wget https://minecraft.glitchless.ru/jres/jre-8u202-macosx-x64.tar.gz -O src/pkg/macos/links/jre.tar.gz

src/pkg/macos/links/jrepath.txt:
	mkdir -p src/pkg/macos/links
	echo 'jre_1.8_mac_x64/Contents/Home/bin/java' > src/pkg/macos/links/jrepath.txt

.PHONY: build
build: src/pkg/macos/links/runner.jar src/pkg/macos/links/jre.tar.gz src/pkg/macos/links/jrepath.txt
	cd src/pkg && gcc -I. macos/pkg.c macos/pkg.s pkg_generic.c util.c -lz -o ../../build/pkg-macos

.PHONY: build-app
build-app: build
	mkdir -p build/Minecraft.app/Contents/{MacOS,Resources}
	cp src/pkg/macos/app/Info.plist build/Minecraft.app/Contents/Info.plist
	cp build/pkg-macos build/Minecraft.app/Contents/MacOS/glitchless-minecraft
	cp src/pkg/macos/app/Resources/icon.icns build/Minecraft.app/Contents/Resources/icon.icns

.PHONY: clean
clean:
	rm -rf src/pkg/macos/links/runner.jar src/pkg/macos/links/jre.tar.gz src/pkg/macos/links/jrepath.txt build/pkg-macos build/Minecraft.app

