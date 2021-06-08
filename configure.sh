#!/usr/bin/bash

deps="mysql mysql-workbench x-terminal-emulator gcc make composer php apache2"
b=true

type mysql &> /dev/null

for dep in $deps; do
    echo "checking ${dep}..."
    if ! type $dep > /dev/null; then
        echo "missing dependency: ${dep} ⭕"
        b=false
    else 
        echo "${dep} ☑️ "
    fi
done

if [ $(cat /proc/1/comm) == systemd ] ; then
    echo "init system: systemd ☑️ "
else 
    echo "systemd required ⭕"
    b=false
fi


echo "checking libgtk-3.dev"

ldconfig -p | grep "libgtk-3.so" > /dev/null

if  [ $? -eq 0 ] ; then
    echo "libgtk-3.dev ☑️ "
else 
    echo "libgtk-3.so missing ⭕"
    b=false
fi

if [ $b == true ] ; then
    echo "checking done"
else
    echo "missing some dependencies"
fi