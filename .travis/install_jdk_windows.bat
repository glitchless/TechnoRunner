git clone https://github.com/portapps/portapps ${TRAVIS_BUILD_DIR}/../portapps
source ${TRAVIS_BUILD_DIR}/../portapps/.travis/prepare.sh
bash ${TRAVIS_BUILD_DIR}/../portapps/.travis/java.sh
set