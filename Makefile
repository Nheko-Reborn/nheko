
debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
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

linux-deploy:
	./.ci/linux/deploy.sh
	./.ci/linux/create-packages.sh

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

clean:
	rm -rf build
