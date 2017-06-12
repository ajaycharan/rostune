/*
 * BSD 3-Clause License
 * Copyright (c) 2017, NCSR "Demokritos"
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions at the
 * bottom of this file are met.
 */

#include "nodestats.h"

namespace nodestats {

int getPid( std::string nodeName )
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = "rostune";
  args[1] = nodeName;

  std::string url;
  if( ros::master::execute( "lookupNode", args, result, payload, true ) ) {
    // results are:
    // 0: error code, int, eg, <value><i4>1</i4></value>. 1 is OK, -1,0 is bad.
    // 1: statusMessage, string, eg <value>Some message</value>
    // 2: url, string, eg, <value>http://myhost:6666</value>
    int err = static_cast<int>( result[0] );
    if( err = 1 ) {
      url = static_cast<std::string>( result[2] );
    }
    else {
      return -1;
    }
  }
  else {
    return -1;
  }

  ros::WallDuration tmout( 5, 0 );
  if( execute_at( url, "getPid", args, result, payload, true, tmout ) ) {
    // results are:
    // 0: error code, int, eg, <value><i4>1</i4></value>. 1 is OK, -1,0 is bad.
    // 1: statusMessage, string, eg <value>Some message</value>
    // 2: pid, int, eg, <value><i4>666</i4></value>
    return static_cast<int>( result[2] );
  }
  else {
    return -1;
  }
}


void cpuload( int pid,
      uint64_t &cputime, uint64_t &all_mem, uint64_t &resident_mem )
{
  FILE *fp;
  std::stringstream pathname;
  pathname << "/proc/" << pid << "/stat";
  fp = fopen( pathname.str().c_str(), "r" );

  long clockrate = sysconf( _SC_CLK_TCK );
  
  unsigned long utime, stime, cutime, cstime, vsize;
  long num_threads, rss;
  unsigned long long starttime;
  
  if( fp != NULL ) {
    char line[1024];
    fgets( line, sizeof line, fp );
    ROS_DEBUG( "proc line, %d bytes: %s", strlen(line), line );
    sscanf( line,  
    "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %lu %lu %*ld %*ld %ld %*ld %llu %lu %ld %*lu %*lu %*s",
    &utime, &stime, &cutime, &cstime, &num_threads, &starttime, &vsize, &rss );
    fclose( fp );

    ROS_DEBUG( "Extracted: utime %lu, stime %lu, cutime %lu, cstime %lu, num_threads: %ld starttime: %llu vsize: %lu rss: %ld", utime, stime, cutime, cstime, num_threads, starttime, vsize, rss );
    cputime = 1000*(cutime + cstime + utime + stime)/clockrate;
    all_mem = vsize;
    resident_mem = rss * sysconf( _SC_PAGE_SIZE );
  }
  else {
    cputime = 0;
    all_mem = 0;
    resident_mem = 0;
  }    
  ROS_DEBUG( "Clockrate: %ld page size %d", clockrate, sysconf(_SC_PAGE_SIZE) );
  ROS_DEBUG( "Computed: cputime (msec) %lu, allmem: %lu, resmem %lu",
     cputime, all_mem, resident_mem );
}

/*
From http://man7.org/linux/man-pages/man5/proc.5.html
(14) utime  %lu
   Amount of time that this process has been scheduled
   in user mode, measured in clock ticks (divide by
   sysconf(_SC_CLK_TCK)).  This includes guest time,
   guest_time (time spent running a virtual CPU, see
   below), so that applications that are not aware of
   the guest time field do not lose that time from
   their calculations.

(15) stime  %lu
   Amount of time that this process has been scheduled
   in kernel mode, measured in clock ticks (divide by
   sysconf(_SC_CLK_TCK)).

(16) cutime  %ld
   Amount of time that this process's waited-for
   children have been scheduled in user mode, measured
   in clock ticks (divide by sysconf(_SC_CLK_TCK)).
   See also times(2).)  This includes guest time,
   cguest_time (time spent running a virtual CPU, see
   below).

(17) cstime  %ld
   Amount of time that this process's waited-for
   children have been scheduled in kernel mode,
   measured in clock ticks (divide by
   sysconf(_SC_CLK_TCK)).

(20) num_threads  %ld
   Number of threads in this process (since Linux 2.6).
   Before kernel 2.6, this field was hard coded to 0 as
   a placeholder for an earlier removed field.

(23) vsize  %lu
   Virtual memory size in bytes.

(24) rss  %ld
   Resident Set Size: number of pages the process has
   in real memory.  This is just the pages which count
   toward text, data, or stack space.  This does not
   include pages which have not been demand-loaded in,
   or which are swapped out.

              (27) endcode  %lu  [PT]
                        The address below which program text can run.

              (28) startstack  %lu  [PT]
                        The address of the start (i.e., bottom) of the
                        stack.

              (29) kstkesp  %lu  [PT]
                        The current value of ESP (stack pointer), as found
                        in the kernel stack page for the process.

              (30) kstkeip  %lu  [PT]
                        The current EIP (instruction pointer).

              (31) signal  %lu
                        The bitmap of pending signals, displayed as a
                        decimal number.  Obsolete, because it does not
                        provide information on real-time signals; use
                        /proc/[pid]/status instead.

              (32) blocked  %lu
                        The bitmap of blocked signals, displayed as a
                        decimal number.  Obsolete, because it does not
                        provide information on real-time signals; use
                        /proc/[pid]/status instead.

              (33) sigignore  %lu
                        The bitmap of ignored signals, displayed as a
                        decimal number.  Obsolete, because it does not
                        provide information on real-time signals; use
                        /proc/[pid]/status instead.

              (34) sigcatch  %lu
                        The bitmap of caught signals, displayed as a decimal
                        number.  Obsolete, because it does not provide
                        information on real-time signals; use
                        /proc/[pid]/status instead.

              (35) wchan  %lu  [PT]
                        This is the "channel" in which the process is
                        waiting.  It is the address of a location in the
                        kernel where the process is sleeping.  The
                        corresponding symbolic name can be found in
                        /proc/[pid]/wchan.

              (36) nswap  %lu
                        Number of pages swapped (not maintained).

              (37) cnswap  %lu
                        Cumulative nswap for child processes (not
                        maintained).

              (38) exit_signal  %d  (since Linux 2.1.22)
                        Signal to be sent to parent when we die.

              (39) processor  %d  (since Linux 2.2.8)
                        CPU number last executed on.

              (40) rt_priority  %u  (since Linux 2.5.19)
                        Real-time scheduling priority, a number in the range
                        1 to 99 for processes scheduled under a real-time
                        policy, or 0, for non-real-time processes (see
                        sched_setscheduler(2)).

              (41) policy  %u  (since Linux 2.5.19)
                        Scheduling policy (see sched_setscheduler(2)).
                        Decode using the SCHED_* constants in linux/sched.h.

                        The format for this field was %lu before Linux
                        2.6.22.

              (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
                        Aggregated block I/O delays, measured in clock ticks
                        (centiseconds).

              (43) guest_time  %lu  (since Linux 2.6.24)
                        Guest time of the process (time spent running a
                        virtual CPU for a guest operating system), measured
                        in clock ticks (divide by sysconf(_SC_CLK_TCK)).

              (44) cguest_time  %ld  (since Linux 2.6.24)
                        Guest time of the process's children, measured in
                        clock ticks (divide by sysconf(_SC_CLK_TCK)).

              (45) start_data  %lu  (since Linux 3.3)  [PT]
                        Address above which program initialized and
                        uninitialized (BSS) data are placed.

              (46) end_data  %lu  (since Linux 3.3)  [PT]
                        Address below which program initialized and
                        uninitialized (BSS) data are placed.

              (47) start_brk  %lu  (since Linux 3.3)  [PT]
                        Address above which program heap can be expanded
                        with brk(2).

              (48) arg_start  %lu  (since Linux 3.5)  [PT]
                        Address above which program command-line arguments
                        (argv) are placed.

              (49) arg_end  %lu  (since Linux 3.5)  [PT]
                        Address below program command-line arguments (argv)
                        are placed.

              (50) env_start  %lu  (since Linux 3.5)  [PT]
                        Address above which program environment is placed.

              (51) env_end  %lu  (since Linux 3.5)  [PT]
                        Address below which program environment is placed.

              (52) exit_code  %d  (since Linux 3.5)  [PT]
                        The thread's exit status in the form reported by
                        waitpid(2).
*/



bool execute_at( std::string url, const std::string& method, const XmlRpc::XmlRpcValue& request, XmlRpc::XmlRpcValue& response, XmlRpc::XmlRpcValue& payload, bool wait_for_master, ros::WallDuration g_retry_timeout )
{
  ros::WallTime start_time = ros::WallTime::now();
  std::string master_host;
  uint32_t master_port;
  parseUrl( url, master_host, master_port );
  XmlRpc::XmlRpcClient *c = ros::XMLRPCManager::instance()->getXMLRPCClient( master_host, master_port, "/" );
  bool printed = false;
  bool slept = false;
  bool ok = true;
  bool b = false;
  do
    {
     {
 #if defined(__APPLE__)
       boost::mutex::scoped_lock lock(g_xmlrpc_call_mutex);
 #endif

       b = c->execute(method.c_str(), request, response);
     }

     ok = !ros::isShuttingDown() && !ros::XMLRPCManager::instance()->isShuttingDown();

     if( !b && ok ) {
       if( !printed && wait_for_master ) {
         ROS_ERROR("[%s] Failed to contact master at [%s:%d].  %s", method.c_str(), master_host.c_str(), master_port, wait_for_master ? "Retrying..." : "");
         printed = true;
       }

       if( !wait_for_master ) {
         ros::XMLRPCManager::instance()->releaseXMLRPCClient(c);
         return false;
       }

       // UGLY HACK: this should be the global ros::WallTime::g_retry_timeout, but it
       // not visible from here. Should be declared in ros/master.h so I can read it.
       if( !g_retry_timeout.isZero() && (ros::WallTime::now() - start_time) >= g_retry_timeout ) {
         ROS_ERROR("[%s] Timed out trying to connect to the master after [%f] seconds", method.c_str(), g_retry_timeout.toSec());
         ros::XMLRPCManager::instance()->releaseXMLRPCClient(c);
         return false;
       }

       ros::WallDuration(0.05).sleep();
       slept = true;
     }
     else {
       if (!ros::XMLRPCManager::instance()->validateXmlrpcResponse(method, response, payload)) {
        ros::XMLRPCManager::instance()->releaseXMLRPCClient(c);
        return false;
       }
       
       break;
     }

     ok = !ros::isShuttingDown() && !ros::XMLRPCManager::instance()->isShuttingDown();
    } while(ok);

  if (ok && slept) {
    ROS_INFO("Connected to master at [%s:%d]", master_host.c_str(), master_port);
  }

  ros::XMLRPCManager::instance()->releaseXMLRPCClient(c);

  return b;
}
 


void parseUrl( std::string url, std::string& host, uint32_t& port )
{
  std::string http( "http://" );
  if( url.compare(0, http.size(), http) == 0 ) {
    unsigned int pos = url.find_first_of("/:", http.size());

    if (pos == std::string::npos) {
      // No port or path
      pos = url.size();
    }
    
    host = url.substr( http.size(), pos-http.size() );
    
    if( pos < url.size() && url[pos] == ':' ) {
      // A port is provided
      unsigned int ppos = url.find_first_of("/", pos);
      if (ppos == std::string::npos) {
        // No path provided, assume port is rest of string
        ppos = url.size();
      }
      std::string strPort = url.substr( pos+1, ppos-pos-1 );
      if( strPort.size() > 0 ) {
        port = atoi( strPort.c_str() );
      }
      else { port = 0; }
    }
    else {
      port = 0;
    }

  }
  else {
    host.empty(); port = 0;
  }
}


} // namespace nodestats


/*
BSD 3-Clause License
Copyright (c) 2017, NCSR "Demokritos"
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

 * Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
