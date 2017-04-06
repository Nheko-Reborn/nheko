run: build
	./build/nheko

debug:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@make -C build

release-debug:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@make -C build

clean:
	rm -rf build

.PHONY: build
