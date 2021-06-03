#!/bin/bash

# insert/update hosts entry

action=$1

# find existing instances in the host file and save the line numbers
if [ $action = "add" ]; then
    ip_address=$2
    host_name=$3
    host_file=$4

    matches_in_hosts="$(grep -nw "${host_name}    #laragolas!#" $host_file | cut -f1 -d:)"
    host_entry="${ip_address}    ${host_name}    #laragolas!#"

    if [ ! -z "$matches_in_hosts" ]; then
        while read -r line_number; do
            sudo sed -i "${line_number}s/.*/${host_entry} /" $host_file 
        done <<< $matches_in_hosts
    else
        echo "$host_entry" | sudo tee -a $host_file > /dev/null
    fi

elif [ $action = "delete" ] ; then
    ip_address=$2
    host_name=$3
    host_file=$4

    matches_in_hosts="$(grep -nw "${host_name}    #laragolas!#" $host_file | cut -f1 -d:)"
    host_entry="${ip_address}    ${host_name}    #laragolas!#"



    if [ ! -z "$matches_in_hosts" ]; then
        while read -r line_number; do
            sudo sed -i "${line_number}d" $host_file 
        done <<< $matches_in_hosts
    fi
elif [ $action = "port" ] ; then
    port=$2
    file=$3
    nums=$(grep -nw '#laragolas!#' $file | cut -f1 -d:)
    if [ ! -z "${nums}" ]; then
    while read -r line_number; do
        sudo sed -i "${line_number}d" $file
    done <<< "$nums" 
    fi
    echo "Listen ${port} #laragolas!#" >> $file
fi
