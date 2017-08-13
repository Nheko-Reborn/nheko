APP_NAME = nheko

MAC_DIST_DIR = dist/MacOS
APP_TEMPLATE = $(MAC_DIST_DIR)/Nheko.app

SRC := $(shell find include src -type f -type f \( -iname "*.cc" -o -iname "*.h" \))


# Linux specific helpers
debug:
	@cmake -DBUILD_TESTS=OFF -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

release-debug:
	@cmake -DBUILD_TESTS=OFF -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build

test:
	@cmake -DBUILD_TESTS=ON -H. -GNinja -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build
	@cd build && GTEST_COLOR=1 ctest --verbose

app: release-debug $(APP_TEMPLATE)
	@cp -fp ./build/$(APP_NAME) $(APP_TEMPLATE)/Contents/MacOS
	@echo "Created '$(APP_NAME).app' in '$(APP_TEMPLATE)'"

app-install: app
	cp -rf $(APP_TEMPLATE) /Applications/Nheko.app

dmg: app
	hdiutil create $(MAC_DIST_DIR)/Nheko.dmg \
		-volname "$(APP_NAME)" \
		-fs HFS+ \
		-srcfolder $(MAC_DIST_DIR) \
		-ov -format UDZO

run:
	@./build/nheko

lint:
	@clang-format -i $(SRC)


clean:
	rm -rf build

.PHONY: build app dmg
