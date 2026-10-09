// MimiClaw-OrangePi —— AI Agent × Edge Computing
// 占位入口：仅用于让 CI 真实执行 build / vet / test。
// 真正的 Agent Runtime 落地时替换此文件。
package main

import (
	"fmt"
	"runtime"
)

func main() {
	fmt.Printf("MimiClaw-OrangePi %s/%s built with %s\n", runtime.GOOS, runtime.GOARCH, runtime.Version())
}
