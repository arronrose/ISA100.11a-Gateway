#! /bin/sh

cd ${0%/*}/..
absolute_path=$(pwd)

#
#include files
#
if [ ! -f $absolute_path/etc/log.sh ]; then
	echo "Cannot find script for log=$absolute_path/etc/log.sh"
	exit 1
fi

LOG_DIR="/tmp/logs"


mkdir -p "${LOG_DIR}"
if [ ! -d "${LOG_DIR}" ]; then
    exit 1
fi

source $absolute_path/etc/log.sh ${LOG_DIR}/mh_run.log


operation=$1
daemon=$2
shift 2
daemon_args="$*"

RUN_PID_FILE=/tmp/Monitor_Host1412_run.pid
DAEMON_PID_FILE=/tmp/Monitor_Host1412_daemon.pid


# check if the daemon is running
# check if the daemon is running
check_is_run() {
    pidfile=$1
    if [ -f $pidfile ] ; then
        pid2check=`cat $pidfile`
        FOUND=`ps | tr -s ' ' | sed 's/^\ *//g' | cut -f1 -d ' '| grep '[0-9]\+' | grep "$pid2check" 2>&1`
        echo "FOUND:${FOUND}"
        if [ "${FOUND}" != "" ] ; then
            return 0 #true
        fi
    fi
    return 1 #false
}

# create a pid file from path + PID
create_pid_file() {
    pidfile=$1
    pid=$2

    log_debug "create pid_file=${pidfile} with pid=${pid}"

    mkdir -p ${pidfile%/*} #make sure that dir exists
    if ! (echo "$pid" > ${pidfile}) ; then
	return 1
    fi
    return 0
}

# kill a proces by pid_file
kill_by_pid_file() {
    pidfile=$1
    signal=$2

    if [ ! -f $pidfile ]; then
	return 0
    fi

    pid2kill=`cat $pidfile`
    log_debug "Killing PID=$pid2kill with signal=$signal ..."
    if ! (kill $signal $pid2kill 2>&1) ; then
	log_error "Failed to kill PID=$pid2kill..."
	return 1
    fi

		echo "killed" > $pidfile
    return 0 #always succeeds
}

# contains the running loop;
#  running loop makes sure the the daemon is running (even after a crash)
do_run() {
    daemon=$1
    shift 1

    while true; do
	log_info "Daemon='$daemon' starting ..."

	mkdir -p ${LOG_DIR}

	(${daemon} $* > /dev/null) &
	pid=$!
	if ! create_pid_file $DAEMON_PID_FILE $pid ; then
	    log_error "Failed to create daemon_pid_file=${DAEMON_PID_FILE} error_code=$? !"
	fi
	log_info "Daemon='$daemon' started with PID=$pid"

	#trap "wait $pid" SIGTERM SIGINT
	wait $pid
	log_info "Daemon='$daemon' killed or crashed!"
	sleep 5
    done

    #rm -r $daemon_run_pid
}

# starts the daemon
try_start() {
    daemon=$1
    shift 1

    log_debug "check_is_run $RUN_PID_FILE"
    if  check_is_run $RUN_PID_FILE ; then
	log_info "Daemon '${daemon}' already started!"
	return 0
    fi

    if [ ! -x $daemon ]; then
	log_error "Daemon '${daemon}' is not executable !"
	return 1
    fi

    #just check if we can create pid files
    if ! create_pid_file $RUN_PID_FILE ; then
	log_error "Failed to creat run_pid_file=${RUN_PID_FILE}"
	return 1
    fi
    if ! create_pid_file $DAEMON_PID_FILE ; then
	log_error "Failed to creat daemon_pid_file=${DAEMON_PID_FILE}"
	return 1
    fi

    log_info "Starting run process into directory=$(pwd)"

    (do_run $daemon $*) &

    if ! create_pid_file $RUN_PID_FILE $! ; then
	log_error "Failed to creat run_pid_file=${RUN_PID_FILE}"
	return 1
    fi

    return 0
}

# stops the daemon
try_stop() {
    daemon=$1

    if ! check_is_run $RUN_PID_FILE ; then
	log_info "Daemon '$daemon' not started!"
	#rm -f $RUN_PID_FILE $DAEMON_PID_FILE #clear pid files if exists
	return 0
    fi

    if ! kill_by_pid_file $RUN_PID_FILE -9 ; then
	log_error "Failed to kill pid_file=${RUN_PID_FILE}"
	return 1
    fi

    if ! kill_by_pid_file $DAEMON_PID_FILE -9 ; then
	log_error "Failed to kill pid_file=${DAEMON_PID_FILE}"
	return 1
    fi

    return 0
}


#
# main program
#
case "${operation}" in
	'start')
	if try_start $daemon $daemon_args ; then
		log_info "Run process started successfully run_pid=$(cat $RUN_PID_FILE)"
		exit 0
	else
		log_error "Failed to Run process."
		error_code="$?"
		try_stop
		exit 1
	fi
	;;

	'stop')
	if try_stop $daemon $daemon_args ; then
		log_info "Run process stopped successfully."
		exit 0
	else
		log_error "Failed to stop."
		exit 1
	fi
	;;

	*)
		print_msg "Usage: $SELF start|stop <path-to-exe> <exe-args>"
		exit 0
	;;
esac
