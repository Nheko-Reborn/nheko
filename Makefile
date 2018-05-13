
debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
	@cmake --build build

ci:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

release:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

linux-install:
	cp -f nheko*.AppImage ~/.local/bin

macos-app-install:
	cp -Rf build/nheko.app /Applications

lint:
	./.ci/format.sh

image:
	docker build -t nheko-app-image .

debian-image:
	docker build -f Dockerfile.debian -t nheko-debian-appimage .

linux-deploy:
	./.ci/linux/deploy.sh

macos-deploy:
	./.ci/macos/deploy.sh

docker-app-image: image
	docker run \
		-e CXX=g++-7 \
		-e CC=gcc-7 \
		-v `pwd`:/build nheko-app-image make release
	docker run \
		--privileged \
		-v `pwd`:/build nheko-app-image make linux-deploy

docker-debian-appimage: debian-image
	docker run -v `pwd`:/build nheko-debian-appimage make release
	docker run --privileged -v `pwd`:/build nheko-debian-appimage make linux-deploy

update-translations:
	lupdate \
		-locations relative \
		-Iinclude/dialogs \
		-Iinclude \
		src/ -ts resources/langs/nheko_*.ts -no-obsolete

clean:
	rm -rf build
