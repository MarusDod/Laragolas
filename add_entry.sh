#!/bin/bash

# insert/update hosts entry
ip_address=$1
host_name=$2
host_file=$3
action=$4

# find existing instances in the host file and save the line numbers
matches_in_hosts="$(grep -nw "${host_name}    #laragolas!#" $host_file | cut -f1 -d:)"
host_entry="${ip_address}    ${host_name}    #laragolas!#"

if [ $action = "add" ]; then

    if [ ! -z "$matches_in_hosts" ]; then
        while read -r line_number; do
            sudo sed -i "${line_number}s/.*/${host_entry} /" $host_file 
        done <<< $matches_in_hosts
    else
        echo "$host_entry" | sudo tee -a $host_file > /dev/null
    fi

elif [ $action = "delete" ] ; then
    if [ ! -z "$matches_in_hosts" ]; then
        while read -r line_number; do
            sudo sed -i "${line_number}d" $host_file 
        done <<< $matches_in_hosts
    fi
fi
