package api

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"unsafe"

	"github.com/moutend/VisualNode/pkg/dll"
)

func GetVisualEnable(w http.ResponseWriter, r *http.Request) error {
	var code int32

	dll.ProcSetup.Call(uintptr(unsafe.Pointer(&code)), uintptr(0))

	if code != 0 {
		log.Printf("Failed to call Setup()")
		return fmt.Errorf("Internal error")
	}
	if _, err := io.WriteString(w, "{}"); err != nil {
		log.Println(err)
		return fmt.Errorf("Internal error")
	}
	return nil
}

func GetVisualDisable(w http.ResponseWriter, r *http.Request) error {
	var code int32

	dll.ProcTeardown.Call(uintptr(unsafe.Pointer(&code)))

	if code != 0 {
		log.Println("Failed to call Teardown (code=%v)", code)
		return fmt.Errorf("Internal error")
	}
	if _, err := io.WriteString(w, "{}"); err != nil {
		log.Println(err)
		return fmt.Errorf("Internal error")
	}

	return nil
}
