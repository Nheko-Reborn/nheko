
debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

test:
	@cmake -DBUILD_TESTS=ON -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build
	@cd build && GTEST_COLOR=1 ctest --verbose

linux-appimage:
	@./.ci/linux/deploy.sh

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
