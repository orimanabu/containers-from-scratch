// https://github.com/lizrice/containers-from-scratch

package main

import (
	"fmt"
//	"io/ioutil"
	"os"
	"os/exec"
//	"path/filepath"
//	"strconv"
	"syscall"
)

// go run main.go run <cmd> <args>
func main() {
	fmt.Println("XXX", os.Args[0])
	switch os.Args[1] {
	case "run":
		run1()
	case "run2":
		run2()
	case "child":
		child()
	default:
		fmt.Println("Usage: %v [command]", os.Args[0])
		fmt.Println("Available Commands:")
		fmt.Println("  run\n")
		panic("help")
	}
}

func run1() {
	fmt.Printf("[R] Running %v \n", os.Args[2:])

	cmd := exec.Command("/proc/self/exe", append([]string{"run2"}, os.Args[2:]...)...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.SysProcAttr = &syscall.SysProcAttr{
		Cloneflags:   syscall.CLONE_NEWUSER,
		UidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getuid(),
				Size:        1,
			},
		},
		GidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getgid(),
				Size:        1,
			},
		},
	}

	must(cmd.Run())
}

func run2() {
	fmt.Printf("[R] Running %v \n", os.Args[2:])

	cmd := exec.Command("/proc/self/exe", append([]string{"child"}, os.Args[2:]...)...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.SysProcAttr = &syscall.SysProcAttr{
		Cloneflags:   syscall.CLONE_NEWUTS | syscall.CLONE_NEWPID | syscall.CLONE_NEWNS,
		//Cloneflags:   syscall.CLONE_NEWUTS | syscall.CLONE_NEWPID,
	}

	must(cmd.Run())
}

func changeRootWithChroot() {
	newRoot := "/home/ori/ubuntu-rootfs"
	must(syscall.Chroot(newRoot))
}

func changeRootWithPivotRoot() {
	newRoot := "/home/ori/ubuntu-rootfs"
	oldRoot := "/old_root"

	// 1. mount alpine root file system as a mountpoint, then it can be used to pivot_root
	fmt.Println("#2.1")
	if err := syscall.Mount(newRoot, newRoot, "", syscall.MS_BIND, ""); err != nil {
		fmt.Println("failed to mount new root filesystem: ", err)
		os.Exit(1)
	}

	fmt.Println("#2.2")
	if f, err := os.Stat(newRoot + oldRoot); os.IsNotExist(err) || !f.IsDir() {
		fmt.Printf("%v not exists, creating...\n", newRoot + oldRoot)
		if err := syscall.Mkdir(newRoot+oldRoot, 0700); err != nil {
			fmt.Println("failed to mkdir: ", err)
			os.Exit(1)
		}
	} else {
		fmt.Printf("%v exisgs, skipping...\n", newRoot + oldRoot)
	}

	fmt.Println("#2.3")
	if err := syscall.PivotRoot(newRoot, newRoot+oldRoot); err != nil {
		fmt.Println("failed to pivot root: ", err)
		os.Exit(1)
	}

	fmt.Println("#2.4")
	if err := syscall.Chdir("/"); err != nil {
		fmt.Println("failed to chdir to /: ", err)
		os.Exit(1)
	}

	fmt.Println("#2.5")
	if err := syscall.Mount("proc", "/proc", "proc", 0, ""); err != nil {
		fmt.Println("failed to mount /proc: ", err)
		os.Exit(1)
	}

	/*
	fmt.Println("#2.6")
	// unmount the old root filesystem
	if err := syscall.Unmount(oldRoot, syscall.MNT_DETACH); err != nil {
		fmt.Println("failed to unmount the old root filesystem: ", err)
		os.Exit(1)
	}

	fmt.Println("#2.7")
	if err := os.RemoveAll(oldRoot); err != nil {
		fmt.Println("failed to remove old root filesystem: ", err)
		os.Exit(1)
	}
	*/
}

func child() {
	fmt.Printf("[C] Running %v \n", os.Args[2:])

	cmd := exec.Command(os.Args[2], os.Args[3:]...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Env = []string{"PS1=(inner-ns) "}

	fmt.Printf("#1\n")
	must(syscall.Sethostname([]byte("container")))
	fmt.Printf("#1.9\n")
	must(syscall.Mount("/home/ori/ubuntu-rootfs/etc", "/home/ori/ubuntu-rootfs/home/ubuntu", "bind", syscall.MS_RDONLY|syscall.MS_BIND, ""))

	fmt.Printf("#2\n")
	//changeRootWithChroot()
	changeRootWithPivotRoot()

	fmt.Printf("#3\n")
	must(os.Chdir("/"))

	//fmt.Printf("#4\n")
	//must(syscall.Mount("proc", "proc", "proc", 0, ""))
	defer syscall.Unmount("proc", 0)

	must(syscall.Mount("thing", "mytemp", "tmpfs", 0, ""))
	defer syscall.Unmount("thing", 0)

	fmt.Printf("#5\n");
	must(cmd.Run())

	fmt.Printf("#6\n");
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
