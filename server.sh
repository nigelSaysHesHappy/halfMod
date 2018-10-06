#!/bin/bash

# These will be the screen names used with screen. They are not important, use whatever you like.
hmScreen=halfMod
hsScreen=halfShell

# The server jar file name must match: `minecraft_server.$mcver.jar`
mcver=1.13.1

autorestart=false
restart=false
hmRestart=false
noMod=false
noShell=false
noCraft=false
noRestart=false

# Set these memory variables to your liking
mem_starting=1024M
mem_total=1024M

# Comment/uncomment these to disable/enable them as default
hmQuiet=--quiet
hmDebug=--debug
#hmVerbose=--verbose

while [[ "$1" != "" ]]; do
    case "$1" in
        --auto-restart)
            autorestart=true
            origswitch+=( $1 )
        ;;
        --restart)
            restart=true
        ;;
        --restart-halfmod)
            hmRestart=true
            origswitch+=( $1 )
        ;;
        --halfmod-screen=*)
            hmScreen=$1
            hmScreen=${hmScreen#--halfmod-screen=*}
            origswitch+=( $1 )
        ;;
        --halfshell-screen=*)
            hsScreen=$1
            hsScreen=${hsScreen#--halfshell-screen=*}
            origswitch+=( $1 )
        ;;
        --minecraft-screen=*)
            hsScreen=$1
            hsScreen=${hsScreen#--minecraft-screen=*}
            origswitch+=( $1 )
        ;;
        --mc-ver=*)
            mcver=$1
            mcver=${mcver#--mc-ver=*}
            origswitch+=( $1 )
        ;;
        --world=*)
            world=$1
            world=${world#--world=*}
            origswitch+=( $1 )
        ;;
        --no-halfshell)
            noShell=true
            origswitch+=( $1 )
        ;;
        --no-halfmod)
            noMod=true
            origswitch+=( $1 )
        ;;
        --solo-halfmod)
            noCraft=true
            origswitch+=( $1 )
        ;;
        --stop)
            noCraft=true
            noMod=true
            hmRestart=true
            restart=false
            origswitch+=( $1 )
        ;;
        --stop-halfmod)
            noCraft=true
            noMod=true
            hmRestart=true
            restart=true
            origswitch+=( $1 )
        ;;
        --stop-minecraft)
            noCraft=true
            noMod=true
            origswitch+=( $1 )
        ;;
        --no-restart)
            noRestart=true
            origswitch+=( $1 )
        ;;
        *)
            hmSwitch+=( $1 )
        ;;
    esac
    shift 1
done

if [[ "$world" != "" ]]; then
    sed -i "s/^level-name=.*$/level-name=$world/" server.properties
fi

if ! $noCraft; then
    echo "#!/bin/bash" >launchmc
    if $noShell; then
        echo "java -Xms${mem_starting} -Xmx${mem_total} -jar minecraft_server.${mcver}.jar nogui" >>launchmc
        if $autorestart; then
            echo "sleep 1" >>launchmc
            echo "./server.sh $origswitch --restart" >>launchmc
        fi
    else
        echo "export LD_PRELOAD=\"${PWD}/halfshell\" && java -Xms${mem_starting} -Xmx${mem_total} -jar minecraft_server.${mcver}.jar nogui" >>launchmc
        if $autorestart; then
            echo "sleep 1" >>launchmc
            echo "export LD_PRELOAD=\"\" && ./server.sh $origswitch --restart" >>launchmc
        fi
    fi
fi

if ! $restart; then
    PID=`screen -ls | grep $hsScreen`
    PID=${PID%%.*}
    PID=${PID#    }
    if [[ "$PID" != "" ]]; then
        if $noRestart; then
            noCraft=true
        else
            echo "Killing $hsScreen . . ."
            kill $PID
        fi
    fi
fi

if $hmRestart; then
    PID=`screen -ls | grep $hmScreen`
    PID=${PID%%.*}
    PID=${PID#    }
    if [[ "$PID" != "" ]]; then
        echo "Killing $hmScreen . . ."
        kill $PID
    fi
fi

if $noMod; then
    if ! $noCraft; then
        if $noShell; then
            echo "Launching Minecraft $mcver on screen $hsScreen . . ."
        else
            echo "Launching halfShell + Minecraft $mcver on screen $hsScreen . . ."
        fi
        if $restart; then
            /bin/bash launchmc
        else
            screen -A -m -d -S ${hsScreen} /bin/bash launchmc
        fi
    fi
elif screen -ls | grep "${hmScreen}">/dev/null; then
    echo "halfMod already running on screen $hmScreen . . ."
    if ! $noCraft; then
        if $noShell; then
            echo "Launching Minecraft $mcver on screen $hsScreen . . ."
        else
            echo "Launching halfShell + Minecraft $mcver on screen $hsScreen . . ."
        fi
        if $restart; then
            /bin/bash launchmc
        else
            screen -A -m -d -S ${hsScreen} /bin/bash launchmc
        fi
    fi
else
    echo "Launching halfMod on screen $hmScreen . . ."
    if $autorestart; then
        screen -A -m -d -S ${hmScreen} /bin/bash halfHold.sh $hmDebug $hmQuiet $hmVerbose --mc-version=${mcver} "${hmSwitch[@]}" localhost 9422
    else
        screen -A -m -d -S ${hmScreen} ./halfmod_engine $hmDebug $hmQuiet $hmVerbose --mc-version=${mcver} "${hmSwitch[@]}" localhost 9422
    fi
    # Wait for halfMod to initialize
    while ! [ -f "listo.nada" ]; do
        sleep 1
    done
    rm "listo.nada"
    if ! $noCraft; then
        if $noShell; then
            echo "Launching Minecraft $mcver on screen $hsScreen . . ."
        else
            echo "Launching halfShell + Minecraft $mcver on screen $hsScreen . . ."
        fi
        if $restart; then
            /bin/bash launchmc
        else
            screen -A -m -d -S ${hsScreen} /bin/bash launchmc
        fi
    fi
fi

sleep 1

rm -f launchmc 2>/dev/null

