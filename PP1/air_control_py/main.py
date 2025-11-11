import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
shm_data = []

SHM_NAME = "/air_control"

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
SHM_LENGTH = 3 * ctypes.sizeof(ctypes.c_int)

# Global variables and locks
planes = 0          # planes waiting
takeoffs = 0        # local takeoffs (blocks of 5)
total_takeoffs = 0  # total takeoffs

state_lock = threading.Lock()
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()


def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    name_bytes = SHM_NAME.encode("utf-8")

    # 2. Temporarily adjust the permission mask (umask) so the memory can be
    #    created with appropriate permissions
    old_umask = os.umask(0)

    # 3. Use _libc.shm_open to create the shared memory
    flags = os.O_CREAT | os.O_RDWR
    mode = 0o666
    fd = _libc.shm_open(name_bytes, flags, mode)
    if fd < 0:
        err = ctypes.get_errno()
        os.umask(old_umask)
        raise OSError(err, "shm_open failed")

    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    if _libc.ftruncate(fd, SHM_LENGTH) != 0:
        err = ctypes.get_errno()
        os.umask(old_umask)
        os.close(fd)
        raise OSError(err, "ftruncate failed")

    # 5. Restore the original permission mask (umask)
    os.umask(old_umask)

    # 6. Use mmap to map the shared memory
    mem = mmap.mmap(fd, SHM_LENGTH, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)

    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    data = memoryview(mem).cast("i")

    data[0] = os.getpid()
    data[1] = 0
    data[2] = 0

    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return fd, mem, data


def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    if signum == signal.SIGUSR2:
        planes += 5


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs, shm_data

    while True:
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                break

        krrnttrack = 0
        if runway1_lock.acquire(blocking=False):
            krrnttrack = 1
        elif runway2_lock.acquire(blocking=False):
            krrnttrack = 2
        else:
            time.sleep(0.001)
            continue

        performed_takeoff = False
        just_reached_limit = False

        with state_lock:
            if planes > 0 and total_takeoffs < TOTAL_TAKEOFFS:
                planes -= 1
                takeoffs += 1
                total_takeoffs += 1
                performed_takeoff = True

                if takeoffs == 5:
                    radio_pid = shm_data[1]
                    if radio_pid > 0:
                        os.kill(radio_pid, signal.SIGUSR1)
                    takeoffs = 0

                if total_takeoffs == TOTAL_TAKEOFFS:
                    just_reached_limit = True

        if not performed_takeoff:
            if krrnttrack == 1:
                runway1_lock.release()
            else:
                runway2_lock.release()
            time.sleep(0.001)
            continue

        time.sleep(1)

        if krrnttrack == 1:
            runway1_lock.release()
        else:
            runway2_lock.release()

        # If this thread performed the last takeoff, send SIGTERM to radio and exit
        if just_reached_limit:
            radio_pid = shm_data[1]
            if radio_pid > 0:
                os.kill(radio_pid, signal.SIGTERM)
            return
    return


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""

    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen(
        ["./radio", SHM_NAME],
        preexec_fn=_unblock_sigusr2,
    )
    return process


def main():
    global shm_data

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    signal.signal(signal.SIGUSR2, HandleUSR2)

    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, memory, data = create_shared_memory()
    shm_data = data 

    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    radio_process = launch_radio()
    shm_data[1] = radio_process.pid  # store radio PID in slot 1

    # TODO 9: Create and start takeoff controller threads (STRIPS)
    threads = []
    for i in range(STRIPS):
        t = threading.Thread(target=TakeOffFunction, args=(i,))
        t.start()
        threads.append(t)

    # TODO 10: Wait for all threads to finish their work
    for t in threads:
        t.join()

    radio_process.wait()

    # TODO 11: Release shared memory and close resources
    memory.close()
    os.close(fd)


if __name__ == "__main__":
    main()
