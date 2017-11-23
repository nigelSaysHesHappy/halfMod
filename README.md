# halfShell
halfShell is the real "mod loader". It works behind the scenes to communicate between Minecraft and mods. halfMod is the bundled mod with this package, but you can also create your own mods and connect them to halfShell as well.

halfShell is the process that will launch and wrap around the minecraft_server.jar. It is a shell server of sorts, only IP's defined in the `halfshell.conf` file will be able to connect to this server. `127.0.0.1` is always allowed by default.

Aside from the obvious ability to connect a halfMod process or other mod to it, you can also `telnet` into halfShell to instantly create a portable Minecraft Console.

# halfMod
halfMod is a brilliant mod that allows the loading of `C++` plugins written with the `halfMod API`. The plugins have absolutely no limitations, in theory. Anything you can do in `C++` can be brought to Minecraft. Of course you are limited to direct input and output of the server console. So you cannot use this to create traditional Minecraft mods or plugins. You are still limited to in-game commands, but with the help from `C++`, so much more than simple vanilla commands is possible.

halfMod aims to create server moderators and admins without the need to grant operator status. Admins can be given various levels of authority, allowing exactly only the permissions you care to give each one. halfMod has been heavily influenced by `SourceMod`, from internal workings to direct plugin processing.

halfMod creates default events that can trigger `C++` functions in plugin files ranging from a player connecting to the server to a player dying or even a player completing an advancement.

halfMod also allows custom event triggers using regex patterns, registered `!commands` in chat, and configurable timers.

# Dependencies
Unfortunately halfShell and halfMod are both limited to supported distros of `Linux`.

`g++ 4.9.2` or higher is required to compile/run.

`screen` is a required package to manage the background processes.

All debian based distros work the best.

`Ubuntu` and `Debian` have both been thoroughly tested with success out of box.

`RedHat` distros can require a little more work, but should also work. With the exception of `CentOS`.

`CentOS` is a free unlicensed distro of `RedHat`, because of this, it can be difficult to install `g++ 4.9.2`. It can be done, but with much headache, I highly discourage it.

No other distros have been tested, but if `g++ 4.9.2` and `screen` are installed, then it should work just fine.

# Install
Installation is very easy. Clone the rep and type the following commands into the terminal:
```
cd src/
./buildall --plugins
```

# Running
Once installed, to launch, from the top directory run the `server.sh` script.
You can set the server to auto restart in the event of a crash by providing the `--auto-restart` command line switch.

All `server.sh` command line options:
`--mc-ver=VERSION` Define the version of Minecraft to launch. (Sets the jar file to `minecraft_server.VERSION.jar`. Default is `1.12.2`)

`--world=WORLD` Change the world directory for Minecraft. If not supplied, uses the definition from `server.properties`.

`--restart` Kills halfShell and Minecraft then restarts it. Does not restart halfMod, but if halfMod is not running, will start it.

`--halfmod-screen=SCREEN` Define the name of the screen halfMod is running on. (Do not use unless you have changed the name.)

`--halfshell-screen=SCREEN` Define the name of the screen halfShell and Minecraft are running on. (Do not use unless you have changed the name. Useful if you are running more than one server.)

Any additional switches will be passed as switches to halfMod.


All `halfmod_engine` command line options:
`--version` Does not start halfMod, instead prints version info.

`--verbose` Print more output in the halfMod console.

`--quiet` Tells halfMod not to print what it sends to Minecraft. (Recommended)

`--debug` Print debug output in the halfMod console.

`--mc-version=VERSION` Tell halfMod which version of Minecraft to expect. Will be overwritten if the server outputs a different value. (`server.sh` will set this to whichever version it launched.)

`--mc-world=WORLD` Tell halfMod which world has been loaded. Will be overwritten if the server outputs a different value.

`--log-bans` Set halfMod to log bans.

`--log-kicks` Set halfMod to log kicks.

`--log-op` Set halfMod to log oper status grants/removals.

`--log-whitelist` Set halfMod to log whitelist changes.
