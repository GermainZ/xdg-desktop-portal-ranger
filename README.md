# xdg-desktop-portal-ranger

[xdg-desktop-portal] backend for choosing files with ranger.
Based on [xdg-desktop-portal-wlr] (xpdw).

## Building

```sh
meson build
ninja -C build
```

## Installing

### From Source

```sh
ninja -C build install
```


## Running

Make sure `XDG_CURRENT_DESKTOP` is set and imported into D-Bus.

When correctly installed, xdg-desktop-portal should automatically invoke
xdg-desktop-portal-ranger when needed.

To use with Firefox, launch Firefox as such: `GTK_USE_PORTAL=1 firefox`.

### Configuration

See `man 5 xdg-desktop-portal-ranger`.

### Manual startup

At the moment, some command line flags are available for development and
testing. If you need to use one of these flags, you can start an instance of
xdpw using the following command:

```sh
xdg-desktop-portal-ranger -r [OPTION...]
```

To list the available options, you can run `xdg-desktop-portal-ranger --help`.

## License

MIT

[xdg-desktop-portal]: https://github.com/flatpak/xdg-desktop-portal
[xdg-desktop-portal-wlr]: https://github.com/emersion/xdg-desktop-portal-wlr
