#!/bin/bash

hmScreen=halfMod
hsScreen=halfShell
mcver=1.12.2
autorestart=false
restart=false
origswitch="$@"

while [[ "$1" != "" ]]; do
	case "$1" in
		--auto-restart)
			autorestart=true
		;;
		--restart)
			restart=true
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

echo "Launching Minecraft + halfShell with halfMod . . ."
sleep 1

echo "#!/bin/bash" >launchmc
echo "stdbuf -oL -eL java -Xms1024M -Xmx1024M -jar minecraft_server.${mcver}.jar nogui" >>launchmc
echo "echo \"]:-:-:-:[###[THREAD COMPLETE]###]:-:-:-:[\"" >>launchmc
echo "#!/bin/bash" >launchhs
if $autorestart; then
	echo "/bin/bash launchmc | { ./halfshell ${hsScreen} ${mcver} ${hsswitch[@]}; ./server.sh $origswitch --restart; }" >>launchhs
else
	echo "/bin/bash launchmc | ./halfshell ${hsScreen} ${mcver} ${hsswitch[@]}" >>launchhs
fi

if ! $restart; then
	PID=`screen -ls | grep halfShell`
	PID=${PID%%.*}
	PID=${PID#	}
	if [[ "$PID" != "" ]]; then
		echo "Killing old halfShell . . ."
		kill $PID
	fi
fi
if screen -ls | grep "${hmScreen}">/dev/null; then
	echo "halfMod already running . . ."
	echo "Launching halfShell . . ."
	screen -A -m -d -S ${hsScreen} /bin/bash launchhs
else
	echo "Launching halfMod . . ."
	screen -A -m -d -S ${hmScreen} ./halfmod_engine "${hsswitch[@]}" --mc-version=${mcver} localhost 9422
	while ! [ -f "listo.nada" ]; do
		sleep 1
	done
	rm "listo.nada"
	echo "Launching halfShell . . ."
	screen -A -m -d -S ${hsScreen} /bin/bash launchhs
	# 1.12.2 ${hsScreen}
#	screen -A -m -d -S ${hsScreen} ./launchhs.sh
fi

sleep 1

rm -f launchmc 2>/dev/null
rm -f launchhs 2>/dev/null

