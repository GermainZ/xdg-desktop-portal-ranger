xdg-desktop-portal-ranger(5)

# NAME

xdg-desktop-portal-ranger - an xdg-desktop-portal backend to choose files with
ranger

# DESCRIPTION

xdg-desktop-portal-ranger (or xdpr for short) allows applications to choose
files via xdg-desktop-portal using ranger.

xdpr will try to load the configuration file from these locations:

- $XDG_CONFIG_HOME/xdg-desktop-portal-ranger/$XDG_CURRENT_DESKTOP
- $XDG_CONFIG_HOME/xdg-desktop-portal-ranger/config
- /etc/xdg/xdg-desktop-portal-ranger/$XDG_CURRENT_DESKTOP
- /etc/xdg/xdg-desktop-portal-ranger/config

_$XDG_CONFIG_HOME_ defaults to _~/.config_.
_$XDG_CURRENT_DESKTOP_ can be a colon seperated list. Each element of that list will be tried.

The configuration files use the INI file format. Example:

```
[filechooser]
term_cmd=/usr/bin/kitty
```

# SCREENCAST OPTIONS

These options need to be placed under the **[screencast]** section.

**term_cmd** = _command_
	Command to execute for the terminal.

	By default, this is set to "/usr/bin/kitty", which means `/usr/bin/kitty
	ranger [options]` will be invoked for choosing files.

# SEE ALSO

**ranger**(1)
