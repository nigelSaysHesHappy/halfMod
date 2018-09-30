#!/bin/bash

# These will be the screen names used with screen. They are not important, use whatever you like.
hmScreen=halfMod
hsScreen=halfShell

# The server jar file name must match: `minecraft_server.$mcver.jar`
mcver=1.13.1

autorestart=false
restart=false
hmRestart=false

# Set these memory variables to your liking
mem_starting=1024M
mem_total=1024M

# Comment/uncomment these to disable/enable them as default
hmQuiet=--quiet
hmDebug=--debug
#hmVerbose=--verbose

origswitch="$@"

while [[ "$1" != "" ]]; do
	case "$1" in
		--auto-restart)
			autorestart=true
		;;
		--restart)
			restart=true
		;;
		--restart-halfmod)
		    hmRestart=true
		    hsswitch+=( $1 )
	    ;;
		--halfmod-screen=*)
			hmScreen=$1
			hmScreen=${hmScreen#--halfmod-screen=*}
		;;
		--halfshell-screen=*)
			hsScreen=$1
			hsScreen=${hsScreen#--halfshell-screen=*}
		;;
		--mc-ver=*)
			mcver=$1
			mcver=${mcver#--mc-ver=*}
		;;
		--world=*)
			world=$1
			world=${world#--world=*}
		;;
		*)
			hsswitch+=( $1 )
		;;
	esac
	shift 1
done

if [[ "$world" != "" ]]; then
	sed -i "s/^level-name=.*$/level-name=$world/" server.properties
fi

#echo "Launching Minecraft + halfShell with halfMod . . ."
#sleep 1

echo "#!/bin/bash" >launchmc
echo "extern LD_PRELOAD=\"${PWD}/halfshell\" && java -Xms${mem_starting} -Xmx${mem_total} -jar minecraft_server.${mcver}.jar nogui" >>launchmc
if $autorestart; then
    echo "./server.sh $origswitch --restart" >>launchmc
fi

if ! $restart; then
	PID=`screen -ls | grep $hsScreen`
	PID=${PID%%.*}
	PID=${PID#	}
	if [[ "$PID" != "" ]]; then
		echo "Killing old halfShell . . ."
		kill $PID
	fi
fi

if $hmRestart; then
    PID=`screen -ls | grep $hmScreen`
    PID=${PID%%.*}
	PID=${PID#	}
	if [[ "$PID" != "" ]]; then
		echo "Killing old halfMod . . ."
		kill $PID
	fi
fi

if screen -ls | grep "${hmScreen}">/dev/null; then
	echo "halfMod already running . . ."
	echo "Launching halfShell + Minecraft $mcver . . ."
	screen -A -m -d -S ${hsScreen} /bin/bash launchmc
else
	echo "Launching halfMod . . ."
	if $autorestart; then
	    screen -A -m -d -S ${hmScreen} /bin/bash halfHold.sh "${hsswitch[@]}" --debug --mc-version=${mcver} localhost 9422
    else
        screen -A -m -d -S ${hmScreen} ./halfmod_engine $hmDebug $hmQuiet $hmVerbose --mc-version=${mcver} "${hsswitch[@]}" localhost 9422
    fi
    # Wait for halfMod to initialize
	while ! [ -f "listo.nada" ]; do
		sleep 1
	done
	rm "listo.nada"
	echo "Launching halfShell + Minecraft $mcver . . ."
	screen -A -m -d -S ${hsScreen} /bin/bash launchmc
	# 1.12.2 ${hsScreen}
#	screen -A -m -d -S ${hsScreen} ./launchhs.sh
fi

sleep 1

rm -f launchmc 2>/dev/null

