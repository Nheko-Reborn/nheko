nheko
----

The motivation behind the project is to provide a native desktop app for [Matrix] that
feels more like a mainstream chat app ([Riot], Telegram etc) and less like an IRC client.

#### Features

Most of the features you'd expect from a chat application are missing right now
but you can of course receive and send messages in the rooms that you are a member of.

#### Requirements

Building instructions for OSX and Windows will be added.

##### Linux

- Qt5
- CMake v3.1 or greater
- GCC that supports C++11.

###### Arch Linux

```bash
$ sudo pacman -S qt5-base cmake gcc
```

#### Building

Run `make build`. The `nheko` binary will be located in the `build` directory.

#### Contributing

Any kind of contribution to the project is greatly appreciated. You are also
encouraged to open feature request issues.

#### Screens

Here is a screen shot to get a feel for the UI, but things will probably change.

![nheko](https://dl.dropboxusercontent.com/s/u6rsx8rsp1u2sko/screen.png)


#### License

[GPLv3]

[Matrix]:https://matrix.org
[Riot]:https://riot.im
[GPLv3]:https://www.gnu.org/licenses/gpl-3.0.en.html
