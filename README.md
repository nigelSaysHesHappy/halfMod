# About
halfShell and halfMod together create a whole new Minecraft server environment. From adding moderators and admins in vanilla, to very non-vanilla abilities like full game changing mechanics. All possible without any modification to the minecraft server jar file. Both halfShell and halfMod are written to support any version of Minecraft as early as server version `0.1.3` (Alpha 1.0.16_02). However, the included plugins are only written for `1.13`. Regardless... Expect bugs in versions prior to `1.8`. For `1.12*` versions and older use the master branch. This branch is for `1.13+`, but *should* still be backwards compatible. This branch has only been tested on `1.13`.

Join the halfShell Mod Development Discord server: <https://discord.gg/QKEGRQr>

# halfShell
halfShell is the real "mod loader". It works behind the scenes to communicate between Minecraft and mods. halfMod is the bundled mod with this package, but you can also create your own mods and connect them to halfShell as well.

halfShell is a shared object that wraps around the JVM Environment. It creates a shell server of sorts, only IP's defined in the `halfshell.conf` file will be able to connect to this server. `127.0.0.1` is always allowed by default.

Aside from the obvious ability to connect a halfMod process or other mod to it, you can also `telnet` into halfShell to instantly create a portable Minecraft Console.

# halfMod
halfMod is a brilliant mod that allows the loading of `C++` plugins written with the `halfMod API`. The plugins have absolutely no limitations, in theory. Anything you can do in `C++` can be brought to Minecraft. Of course you are limited to direct input and output of the server console. So you cannot use this to create traditional Minecraft mods or plugins. You are still limited to in-game commands, but with the help from `C++`, so much more than simple vanilla commands is possible.

halfMod aims to create server moderators and admins without the need to grant operator status. Admins can be given various levels of authority, allowing exactly only the permissions you care to give each one. halfMod has been heavily influenced by `SourceMod`, from internal workings to direct plugin processing.

halfMod creates default events that can trigger `C++` functions in plugin files ranging from a player connecting to the server to a player dying or even a player completing an advancement.

halfMod also allows custom event triggers using regex patterns, registered `!commands` in chat, and configurable timers.

# Dependencies
Unfortunately halfShell and halfMod are both limited to supported distros of `Linux`.

`clang++-3.8` or higher is required to compile/run.  
If `/usr/bin/clang++-3.8` does not exist, you will need to create a link there that points to your clang binary.

`libc++-dev` package is also required.

`g++` is needed to compile halfShell. Older versions should work just fine. *Untested*

~~`screen` is a required package to manage the background processes.~~  
`screen` is no longer required, however it is still recommended.

All debian based distros work the best.

`Ubuntu` and `Debian` have both been thoroughly tested with success out of box.

`RedHat` distros can require a little more work, but should also work. With the exception of `CentOS`.

~~`CentOS` is a free unlicensed distro of `RedHat`, because of this, it can be difficult to install `g++ 4.9.2`. It can be done, but with much headache, I highly discourage it.~~  
`CentOS` has not been tested since switching to clang.

No other distros have been tested, but if `g++ 4.9.2`, `clang++-3.8`, and `libc++-dev` are installed, then it should work just fine.

# Install
Installation is very easy. Clone the repo and type the following commands into the terminal:
```
cd src/
chmod +x buildall.sh
./buildall.sh --plugins
```

# Running
Once installed, to launch, from the top directory run the `server.sh` script.
You can set the server to auto restart in the event of a crash by providing the `--auto-restart` command line switch.
The script assumes you have `screen` installed. If you want to use something else to handle background processes, you will need to modify the script or make your own.

All `server.sh` command line options:

`--auto-restart` Setup Minecraft and halfMod to automatically restart in the event of a crash or when they stop for any reason.

`--mc-ver=VERSION` Define the version of Minecraft to launch. (Sets the jar file to `minecraft_server.VERSION.jar`. Default is `1.12.2`)

`--world=WORLD` Change the world directory for Minecraft. If not supplied, uses the definition from `server.properties`.

`--restart` Misleading. Used when halfShell triggers an auto restart. Should not be used outside of the halfShell + Minecraft screen.

`--restart-halfmod` Restarts halfMod if it is already running. If used with `--auto-restart`, halfMod will be reloaded each time Minecraft crashes.

`--halfmod-screen=SCREEN` Define the name of the screen to run halfMod from. (Useful if you are running more than one server.)

`--halfshell-screen=SCREEN` Define the name of the screen to run Minecraft from. (Useful if you are running more than one server.)

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

By default, all logging options are enabled.

`--log-off` Disable all logging.

`--log-bans-off` Disable ban logging.

`--log-kicks-off` Disable kick logging.

`--log-op-off` Disable oper status logging.

`--log-whitelist-off` Disable whitelist logging.

If you only wanted to log bans, instead of disabling each one individually, you could do: `--log-off --log-bans`

# Adding Admins
The file `./halfMod/config/admins.conf` contains the admin definitions.

Example file:
```
# halfMod admin config file
# Valid flags are lowercase letters a-z
# Example:
# nigel = abc

MrPoopyButthole = abcefghikmoqrstuvwxy
nigel = z
```
Full list of flags:
```
FLAG_ADMIN        (a) =   Generic admin
FLAG_BAN          (b) =   Can ban/unban
FLAG_CHAT         (c) =   Special chat access
FLAG_CUSTOM1      (d) =   Custom1
FLAG_EFFECT       (e) =   Grant/revoke effects
FLAG_CONFIG       (f) =   Execute server config files
FLAG_CHANGEWORLD  (g) =   Can change world
FLAG_CVAR         (h) =   Can change gamerules and plugin variables
FLAG_INVENTORY    (i) =   Can access inventory commands
FLAG_CUSTOM2      (j) =   Custom2
FLAG_KICK         (k) =   Can kick players
FLAG_CUSTOM3      (l) =   Custom3
FLAG_GAMEMODE     (m) =   Can change players' gamemode
FLAG_CUSTOM4      (n) =   Custom4
FLAG_OPER         (o) =   Grant/revoke operator status
FLAG_CUSTOM5      (p) =   Custom5
FLAG_RSVP         (q) =   Reserved slot access
FLAG_RCON         (r) =   Access to server RCON
FLAG_SLAY         (s) =   Slay/harm players
FLAG_TIME         (t) =   Can modify the world time
FLAG_UNBAN        (u) =   Can unban, but not ban
FLAG_VOTE         (v) =   Can initiate votes
FLAG_WHITELIST    (w) =   Access to whitelist commands
FLAG_CHEATS       (x) =   Commands like give, xp, enchant, etc...
FLAG_WEATHER      (y) =   Can change the weather
FLAG_ROOT         (z) =   Magic flag which grants all flags
```

# More info on everything can be found on the [wiki](https://github.com/nigelSaysHesHappy/halfMod/wiki)
