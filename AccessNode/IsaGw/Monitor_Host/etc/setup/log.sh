
#logging mechanism

log_file=$1
echo "config log_file=$log_file"

log_enable_echo="no"
log_enable_print_caller="yes"
log_enable_debug="no"

if [ "$log_enable_print_caller" == "yes" ]; then
	log_print_caller="caller 0"
else
	log_print_caller=""
fi


log_error() {
	echo "ERROR : $1 [$($log_print_caller)]"
	echo "$(date +'%X') ERROR $$: $1 [$(caller 0)]" >> $log_file
}

log_info() {
	echo "INFO : $1 [$($log_print_caller)]"
	echo "$(date +'%X') INFO $$: $1 [$(caller 0)]" >> $log_file
}

log_debug() {
	if [ "$log_enable_debug" == "yes" ]; then
		[ "$log_enable_echo" == "yes" ] && echo "DEBUG: $1 [$($log_print_caller)]"
		echo "$(date +'%X') DEBUG $$: $1 [$(caller 0)]" >> $log_file
	fi
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

