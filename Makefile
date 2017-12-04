
debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
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

clean:
	rm -rf build

.PHONY: build app dmg
