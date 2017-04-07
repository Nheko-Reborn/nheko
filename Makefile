run: debug
	./build/nheko

debug:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug
	@make -C build -j2

release-debug:
	@cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@make -C build -j2

clean:
	rm -rf build

.PHONY: build
