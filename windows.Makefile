.PHONY: all
all: build

build\libs\TechnoRunner-1.0-SNAPSHOT.jar:
	gradlew.bat fatJar

src\pkg\windows\links\runner.jar: build\libs\TechnoRunner-1.0-SNAPSHOT.jar
	mkdir -p src\pkg\windows\links
	cp build\libs\TechnoRunner-1.0-SNAPSHOT.jar src\pkg\windows\links\runner.jar

src\pkg\windows\links\jre.tar.gz:
	mkdir -p src\pkg\windows\links
	curl https://minecraft.glitchless.ru/jres/jre-8u202-windows-x64.tar.gz --output src\pkg\windows\links\jre.tar.gz

src\pkg\windows\links\jrepath.txt:
	mkdir -p src\pkg\windows\links
	echo jre1.8.0_202\bin\java.exe > src\pkg\windows\links\jrepath.txt

.PHONY: build
build: src\pkg\windows\links\runner.jar src\pkg\windows\links\jre.tar.gz src\pkg\windows\links\jrepath.txt
	gcc -static -I src\pkg src\pkg\windows\pkg.c src\pkg\windows\pkg.s src\pkg\pkg_generic.c src\pkg\util.c -lz -o build\pkg-windows

.PHONY: clean
clean:
	rm src\pkg\windows\links\runner.jar src\pkg\windows\links\jre.tar.gz src\pkg\windows\links\jrepath.txt build\pkg-windows.exe

