Commit: Current  

Fixed bug that required previous revert.  
Fixed small bug with the `urlchat` plugin.  

Commit: 62db81f  

Smaller bug fix  

Commit: d9bd253  

Small revert to fix new bug  

Commit: 2b86084  

Added some color output to `hm_info` and `hm_cvarinfo`.  

Commit: 544b9fb  

Fixed a nasty bug with the `cvar_rules` plugin causing an aggressive halfShell launch loop.
Minor halfMod engine optimizations.  

Plugins changes:
+ `baseplayerinfo`:
+ + Now the `hm_whois` command will only output player IPs to admins that have root access.
+ `mailbox`:
+ + Updated `hm_senditem` for new Item nbt changes.
+ `disco`:
+ + Updated enchantments on armor for new `Enchantments` tag.
+ `cvar_rules`:
+ + Now properly handles empty lines and `#comments` in `./halfMod/config/gamerules.conf`.
+ `statbar`:
+ + Updated for halfMod API changes.
+ + Small bug fixes.  

Commit: a442fcd  

Fixed a few compile errors and updated a few plugins to conform to new methods.  

Commit: 022b1e5  

API Changes:
+ Updated `mcVerInt()` to properly support pre-release snapshot version numbers.
+ Plugins now link the unload event function dynamically. Allows more control, specifically with extensions.
+ `client` in command callbacks is now a const `hmPlayer` ref object instead of a string.
+ + Any function that accepts client as a string, now may also accept a `hmPlayer` object where it makes sense.
+ + + Obviously it doesn't make sense to do `hmGetPlayerUUID(const hmPlayer&)` because if you have the object, you have the UUID.
+ `findCmd`, `findEvent`, `findTimer`, and `findHook` now all return bools, but assign a reference variable to the found object.
+ + No longer a copy.
+ Added `hmGetPlayerPtr` function. Just like `hmGetPlayerInfo` except returns as a pointer to the object.  

Engine Changes:
+ Timer overhaul.
+ + Now when halfMod is idle, it will truely use 0% CPU until receiving data from minecraft, stdin, or a timer is ready to trigger.
+ + Timers are now much more accurate, to a small fraction of milliseconds.
+ + Micro/nanosecond timers are now actually possible for amounts less than 10 microseconds.  

Commit: 47d2437  

Updated `telepearl` plugin to use `hmProcessTargets` for the `hm_telepearl` command  

Commit: 47dfccd  

Updated nbtmap.h
villager_shops plugin is more stable
Ported telepearl plugin from master branch to dev-1.13
skulldrop plugin now has a command to give yourself a player's head
Added new plugin `warp_points`  

Commit: 47d2437  

Added a new library `nbtmap.h`:
+ Fast and easy NBT parsing header-only library
+ Very small, has no error checking at all. Assumes all NBT is valid and works 100% with all valid NBT
+ No type checks or conversions, everything is treated as a string
+ Ideal for parsing `data get` requests from the server, but can also be used to build NBT
+ See `./src/plugins/villager_shops.cpp` for example usage  

API Changes:
+ Added a new event `onPluginsLoaded` triggers before server startup after every extension and plugin has finished loading
+ Added `hmResolveFlag()` and `hmResolveFlags()` both return the integer representation of a flag string  

Plugin Changes:
+ baseplayerinfo.cpp now uses nbtmap.h
+ New plugin: `villager_shops`:
+ + Create survival shops using villagers
+ + Create and restock shops from a chest
+ + All payments made in the shop are tracked and given to the shop owner  

Commit: ad883f7  

Updated halfMod API to v0.2.4
Updated halfMod Engine to v0.1.3
Updated halfShell to v0.3.3
Full changelog: https://github.com/nigelSaysHesHappy/halfMod/blob/1.13-dev/changelog.txt  

halfMod API Changes:
+ Added Console Variables (aka ConVars or cvars)
+ + Defined by plugins or extensions, removed when the defining plugin is unloaded
+ + Act as a global variable that can be changed through the `hm_cvar` command
+ + Simplifies creating customizable variables or settings for plugins and extensions
+ + Can be linked to gamerules
+ + Can have default, min, and max value rules
+ + Stored as a string, converted to float for min/max boundaries
+ + Can be retrieved/stored as string/bool/int/float
+ + Can be hooked to trigger a function when the value has changed (by whatever means)
+ + There are flags that can be OR'd together to control the cvar's behaviour:
+ + + FCVAR_NOTIFY = Clients are notified of changes
+ + + FCVAR_CONSTANT = The cvar is assigned to the default value and cannot be changed
+ + + FCVAR_READONLY = Only internal methods will be able to change the value, gamerule will be able to change it, but hm_cvar will not
+ + + FCVAR_GAMERULE = The cvar is tied to a gamerule of the same name
+ + If linked to a gamerule (with min/max values defined) and set out of bounds, the value will instantly be reverted back within bounds
+ + + This allows a server owner to set the bounds of gamerules so admins cannot change them to unwanted values
+ + For non-gamerule cvars, they simply will not be allowed to change out of bounds
+ + All plugins and extensions have access to all defined cvars
+ + + Multiple plugins/extensions can hook the same cvar
+ + + When creating a new cvar, if it already exists a pointer to it is returned
+ Removed some restrictions on extensions making them more flexible
+ + Extensions can now create timers and hook patterns
+ Added high resolution timers
+ + Timer syntax has changed slightly:
+ + + int createTimer(std::string name, long interval, std::string function, std::string args = "", short type = SECONDS);
+ + + + Type will default to SECONDS. Allowed options:
+ + + + + SECONDS, MILLISECONDS, MICROSECONDS, or NANOSECONDS
+ + + + interval will be the number of `type` to wait before triggering
+ + + + Existing timers will work exactly the same without any needed modification
+ Added new events:
+ + onGamerule:
+ + + Triggers when a gamerule is set or queried
+ + + The smatch list is as follows:
+ + + + 1 = gamerule, 2 = "now" (has changed) or "currently" (query), 3 = value
+ + onGlobalMessage:
+ + + Triggers when the `hmSendMessageAll` command is used from anywhere, after the message is sent
+ + + The smatch list is just: 1 = message
+ + onPrintMessage:
+ + + Triggers when any somewhat private or server related message is printed to the console
+ + + Follows --debug --quiet --verbose rules
+ + + The smatch list is just: 1 = message
+ + onConsoleReceive:
+ + + Triggers every time any message is received from the server that does NOT match any HM_BLOCK_OUTPUT filters
+ + + Basically if the server message will be printed to the console, this will trigger
+ + + The smatch list is just: 0 = message
+ Added function: `hmProcessTargetsPtr`
+ + Functions exactly the same as `hmProcessTargets`, except the target list is populated with pointers to the matching hmPlayer structs
+ + + Allows finding and directly modifying stored player data
+ Added function overloads for every function that has a `string function` argument
+ + Now they also accept function pointers instead of the string  

halfMod Engine Changes:
+ Added three new internal commands:
+ + hm_cvar <cvar> [value]
+ + + View and set the values of ConVars
+ + hm_cvarinfo [page#] | hm_cvarinfo <search criteria> [page#]
+ + + View and search all ConVars
+ + + Works similarly to the `hm_info` command, but without being able to limit to a specific plugin
+ + hm_resetcvar <cvar>
+ + + Reset a ConVar to its default value
+ hm_info now displays internal commands as well
+ + Using `hm_info plugin internal` will display only internal commands
+ Offline players are now allowed to run commands as themself
+ + Used with `hs raw [99:99:99] [Server thread/INFO]: <nigel> !kick %all`
+ + Any command feedback to the user is printed to the console instead
+ Config file can now be partially used in conjunction with the command line arguments
+ By default all logging options are enabled defaultly
+ + Added command line switches to disable specific logging:
+ + + --log-off, --log-bans-off, --log-kicks-off, --log-op-off, --log-whitelist-off
+ + + The old --log-+ switches still exist, useful for example: --log-off --log-bans
+ Now halfMod will disconnect from the server if it receives an invalid handshake  

Technical Changes:
+ Now uses clang++ to compile instead of g++
+ + This fixes a crash caused by a regex overflow
+ + All compile scripts have been updated. Be sure to install clang!
+ Fixed a rare bug with `str_tok` miscalculating token sizes
+ Tons of optimizations for the API, halfMod engine, and halfShell  

halfMod Plugin Changes:
+ New plugin `advbans`:
+ + Ban IP and UUID ranges
+ + + Loosely based off IRC's Ban-Lines like k-line and z-line
+ + + Piggy backs off `admincmds`'s offline timed unban system
+ + + + If the server is offline when a ban should expire, without the `admincmds` plugin, they will not be lifted
+ + + + Timed bans while the server is online do not rely on `admincmds`
+ + Commands:
+ + + `hm_bline <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]`
+ + + + Ban a username:uuid@ip mask
+ + + + + When a user connects that matches the pattern, they will be banned for `minutes`
+ + + `hm_unbline <pattern (username:uuid@ip)>`
+ + + + Remove a bline pattern, but not any of the bans it created
+ + + `hm_kline <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]`
+ + + + Auto kick a username:uuid@ip mask
+ + + + + When a user connects that matches the pattern, they will be kicked
+ + + `hm_unkline <pattern (username:uuid@ip)>`
+ + + + Remove a kline pattern
+ New plugin `cvar_rules`:
+ + Define ConVars for configured gamerules
+ + Gamerules are configured in "./halfMod/config/gamerules.conf"
+ New plugin `flags`:
+ + Dynamically set users flags during the current session
+ New plugin `log`:
+ + User Defined Personal and Global Logs
+ New plugin `motd`:
+ + Welcome Message of the Day
+ + Displays a configurable json or txt file to players on joining the server
+ + + If using txt file `%identifiers` (%all, %me, %r, etc...) are evaluated
+ + Can also execute configured commands when players join
+ New plugin `urlchat`:
+ + Clickable URL's in chat
+ + Sending a url into chat will be converted into a clickable json link
+ + If the url points to a page containing a title/header, that text will be used as the link text instead of the url itself
+ + + Follows all redirects
+ + Requires `curl`. Not the dev package, just that curl itself be installed
+ New plugin `skulldrop`:
+ + Players drop their head when they die
+ + If cvar `drop_head` is true, players drop their head on death
+ + + Default: true
+ Changed `admincmds`:
+ + Added a cvar `default_ban_time`:
+ + + Default number of minutes to ban someone for if no value is provided to the hm_ban command
+ + + Default: 0 (permenant)
+ + Fixed `hm_noclip` for 1.13 changes
+ Changed `disco`:
+ + Added a cvar `disco_time`:
+ + + Change the tempo of the disco! In fractional seconds
+ + + Default: 1.0
+ + Removed `hm_disco_time` command
+ + Now the rate is in milliseconds (disco_time+1000)
+ Changed `geoip`:
+ + Added a cvar `geoip_onconnect`:
+ + + Display player's GeoIP location info when connecting
+ + + Default: false
+ + Now stores the geoip lookup in the player's data struct, displays on `hm_whois` calls
+ Changed `reservedslots`:
+ + Added a cvar `reserved_slots`:
+ + + Set the number of reserved slots
+ + + Default: 1
+ + Removed `hm_reservedslots` command
+ Changed `vote`:
+ + Added a cvar `vote_timeout`:
+ + + Seconds to wait for votes before ending
+ + + Default: 90
+ + Added a cvar `vote_time_add`:
+ + + Seconds to add to the timeout each time a vote is cast
+ + + Default: 60
+ + Fixed a bug with `hm_voterun` being weird with certain parameters  

Added new secret features! (Because I can't remember everything else that has been changed/added)