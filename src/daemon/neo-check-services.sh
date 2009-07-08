#!/bin/bash

source /usr/lib/neoshell.functions

LOCAL_ADMIN_MODE=0

for arg in $(cat /proc/cmdline); do
  optarg=`expr "x$arg" : 'x[^=]*=\(.*\)'`
  case $arg in
    localadmin=1)
      LOCAL_ADMIN_MODE=1 ;;
  esac
done

log()
{
  logger -t "neo-watchdogd" -p daemon.alert $1
}

testall()
{
	local RV=0
	TESTS="syslog watcher telnet https ssh"
	for tst in $TESTS; do
		test_$tst
		RV=$((RV + $?))
	done
		
	return $RV
}

test_telnet()
{
	TLNT_STAT_CMD="/etc/init.d/telnetd status >/dev/null 2>&1"
	if config_exists "service telnet"; then
		addr=$(config_value "service telnet listen-address")
		port=$(config_value "service telnet port")
		if [ -z "$port" ]; then
			port=23
		fi
		if [ -z "$addr" ]; then
			addr=127.0.0.1
		fi
		if [ $LOCAL_ADMIN_MODE -eq 0 ]; then
			if ! $TLNT_STAT_CMD; then
				log "telnetd is not running"
				return 1
			fi
		else
			if $TLNT_STAT_CMD; then
				log "telnetd running in local admin mode"
				return 1
			else
				return 0
			fi
		fi

		if ! echo "" | nc -w 1 $addr $port 2>&1 | grep -q "$HOSTNAME login"; then
			log "wrong telnet answer on $addr:$port"
			return 1
		fi
	fi

	return 0
}

check_http()
{
	addr=$1
	port=$2

	curl -m 2 "$addr:$port" > /dev/null 2>&1
	if [[ $? -ne 0 ]]; then
		log "wrong web server answer on $addr:$port"
		return 1
	fi

	return 0
}

check_https()
{
	addr=$1
	port=$2

	curl -m 2 -k "https://$addr:$port" > /dev/null 2>&1
	if [[ $? -ne 0 ]]; then
		log "wrong web server answer on $addr:$port"
		return 1
	fi

	return 0
}

check_sshd()
{
	addr=$1
	port=$2

	if ! echo "SSH-2.0-tester" | nc -w 1 $addr $port | grep -q "^SSH-2.0-OpenSSH"; then
		log "wrong sshd answer on $addr:$port"
		return 1
	fi

	return 0
}

test_ssh()
{
	local RV=0
	local NO_ADDR_SETUP=1

	if config_has_tags "service ssh address"; then
		NO_ADDR_SETUP=0
	fi

	if [[ "$NO_ADDR_SETUP" -eq 1 ]] && config_exists "interfaces management"; then
		check_sshd 192.168.200.1 22
		RV=$((RV + $?))
		return $RV
	fi

	for addr in $(config_list_tags "service ssh address"); do
		for port in $(config_value "service ssh address $addr port"); do
			check_sshd $addr $port
			RV=$((RV + $?))
		done
	done
	return $RV
}

test_syslog()
{
	if ! /etc/init.d/syslog-ng status >/dev/null 2>&1; then
		log "syslog-ng is not running"
		return 1
	fi

	return 0
}

test_watcher()
{
	if ! /etc/init.d/watcher status >/dev/null 2>&1; then
		log "watcher is not running"
		return 1
	fi

	return 0
}


if [ "$*" = "" ]; then
	testall
	res=$?
else
	res=0
	for tst in $@; do
		test_$tst
		res=$((res + $?))
	done
fi

exit $res
