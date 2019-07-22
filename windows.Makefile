.PHONY: all
all: build


build\libs\TechnoRunner-1.0-SNAPSHOT.jar:
	gradlew.bat fatJar

src\pkg\windows\links\runner.jar: build\libs\TechnoRunner-1.0-SNAPSHOT.jar
	if not exist src\pkg\windows\links md src\pkg\windows\links
	copy build\libs\TechnoRunner-1.0-SNAPSHOT.jar src\pkg\windows\links\runner.jar

src\pkg\windows\links\jre.tar.gz:
	if not exist src\pkg\windows\links md src\pkg\windows\links
	# не работает, но если команду руками запустить в cmd, то она отработает
	curl https://minecraft.glitchless.ru/jres/jre-8u202-windows-x64.tar.gz --output src\pkg\windows\links\jre.tar.gz

src\pkg\windows\links\jrepath.txt:
	if not exist src\pkg\windows\links md src\pkg\windows\links
	echo jre1.8.0_202\bin\java.exe > src\pkg\windows\links\jrepath.txt

.PHONY: build
build: src\pkg\windows\links\runner.jar src\pkg\windows\links\jre.tar.gz src\pkg\windows\links\jrepath.txt
	gcc -static -I src\pkg src\pkg\windows\pkg.c src\pkg\windows\pkg.s src\pkg\pkg_generic.c src\pkg\util.c -lz -o build\pkg-windows

.PHONY: clean
clean:
	del src\pkg\windows\links\runner.jar src\pkg\windows\links\jre.tar.gz src\pkg\windows\links\jrepath.txt build\pkg-windows.exe

