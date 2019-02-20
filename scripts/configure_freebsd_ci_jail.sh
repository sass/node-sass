#!/usr/bin/env sh

set -o xtrace

jailName=$1
skelDirectory=$2
cbsd_workdir=/usr/jails
jail_arch="i386"
jail_ver="11.2"

echo "Installing build dependencies for cbsd"
pkg install -y cbsd

# determine uplink ip address
# determine uplink iface
auto_iface=$( /sbin/route -n get 0.0.0.0 |/usr/bin/awk '/interface/{print $2}' )
my_ipv4=$( /sbin/ifconfig ${auto_iface} | /usr/bin/awk '/inet [0-9]+/{print $2}' )

if [ -z "${my_ipv4}" ]; then
	echo "IPv4 not detected"
	exit 1
fi

echo "Writing '${jailName}' configuration file"
cat > /tmp/${jailName}.jconf << EOF
jname="${jailName}"
path="${cbsd_workdir}/${jailName}"
host_hostname="${jailName}.my.domain"
ip4_addr="${my_ipv4}"
mount_devfs="1"
allow_mount="1"
allow_devfs="1"
allow_nullfs="1"
allow_raw_sockets="1"
mount_fstab="${cbsd_workdir}/jails-fstab/fstab.${jailName}"
arch="${jail_arch}"
mkhostsfile="1"
devfs_ruleset="4"
ver="${jail_ver}"
basename=""
baserw="0"
mount_src="0"
mount_obj="0"
mount_kernel="0"
mount_ports="1"
astart="1"
data="${cbsd_workdir}/jails-data/${jailName}-data"
vnet="0"
applytpl="1"
mdsize="0"
rcconf="${cbsd_workdir}/jails-rcconf/rc.conf_${jailName}"
floatresolv="1"
exec_poststart="0"
exec_poststop=""
exec_prestart="0"
exec_prestop="0"
exec_master_poststart="0"
exec_master_poststop="0"
exec_master_prestart="0"
exec_master_prestop="0"
pkg_bootstrap="1"
interface="0"
jailskeldir="$skelDirectory"
exec_start="/bin/sh /etc/rc"
exec_stop="/bin/sh /etc/rc.shutdown"
EOF

echo "Initializing cbsd environment"
env workdir=${cbsd_workdir} /usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf

echo "Creating ${jailName}"
cbsd jcreate jconf=/tmp/${jailName}.jconf inter=0
cbsd jailscp /etc/resolv.conf ${jailName}:/etc/resolv.conf

cat > ~cbsd/jails-fstab/fstab.${jailName}.local <<EOF
${skelDirectory} /etc/skel nullfs rw 0 0
EOF

cbsd jstart jname=${jailName} inter=0

echo "${jailName} created"
