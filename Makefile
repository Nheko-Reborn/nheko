
debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

release:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

linux-appimage:
	@./.ci/linux/deploy.sh

linux-install:
	cp -f nheko*.AppImage ~/.local/bin

macos-app: release-debug
	@./.ci/macos/deploy.sh

macos-app-install:
	cp -Rf build/nheko.app /Applications

run:
	@./build/nheko

lint:
	@./.ci/format.sh

image:
	docker build -t nheko-app-image .

docker-app-image: image
	docker run \
		-e CXX=g++-7 \
		-e CC=gcc-7 \
		-v `pwd`:/build nheko-app-image make release
	docker run \
		--privileged \
		-v `pwd`:/build nheko-app-image make linux-appimage

clean:
	rm -rf build

.PHONY: build app dmg
