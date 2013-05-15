#! /bin/sh
#logging mechanism

log_file=/tmp/mh_run.log
[ "$1" != "" ] && log_file="$1"

echo "config log_file=$log_file"

log_enable_echo="no"
log_enable_debug="yes"

log_error() {
	[ "${log_enable_echo}" = "yes" ] && echo "ERROR $$: $1"
	echo "$(date +'%D %X') ERROR $$: $1" >> $log_file
}

log_info() {
	[ "${log_enable_echo}" = "yes" ] && echo "INFO $$: $1"
	echo "$(date +'%D %X') INFO $$: $1" >> $log_file
}

log_debug() {
	[ "${log_enable_debug}" = "" ] && return
	[ "$log_enable_echo" == "yes" ] && echo "DEBUG $$: $1"
	echo "$(date +'%D %X') DEBUG $$: $1" >> $log_file
}

if [ ! -f $log_file ]; then

	#create log dir if not exists
	log_dir="$(dirname $log_file)"

	if [ ! -d $log_dir ]; then
		mkdir -p $log_dir
		echo "log dir created:$log_dir"
	fi

	touch $log_file
	chmod 0666 $log_file
fi
