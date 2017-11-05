APP_NAME = nheko

MAC_DIST_DIR = dist/MacOS
APP_TEMPLATE = $(MAC_DIST_DIR)/Nheko.app

# Linux specific helpers
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

app: release-debug $(APP_TEMPLATE)
	@cp -fp ./build/$(APP_NAME) $(APP_TEMPLATE)/Contents/MacOS
	@echo "Created '$(APP_NAME).app' in '$(APP_TEMPLATE)'"

app-install: app
	cp -Rf $(APP_TEMPLATE) /Applications/

dmg: app
	hdiutil create $(MAC_DIST_DIR)/Nheko.dmg \
		-volname "$(APP_NAME)" \
		-fs HFS+ \
		-srcfolder $(MAC_DIST_DIR) \
		-ov -format UDZO

run:
	@./build/nheko

lint:
	@./.ci/format.sh

clean:
	rm -rf build

.PHONY: build app dmg
