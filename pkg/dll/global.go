package dll

import (
	"syscall"
)

var (
	dll = syscall.NewLazyDLL("VisualNode.dll")

	ProcSetup                   = dll.NewProc("Setup")
	ProcTeardown                = dll.NewProc("Teardown")
	ProcSetHighlightRectangle   = dll.NewProc("SetHighlightRectangle")
	ProcClearHighlightRectangle = dll.NewProc("ClearHighlightRectangle")
)
