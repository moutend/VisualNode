package dll

import (
	"syscall"
)

var (
	dll = syscall.NewLazyDLL("VisualNode.dll")

	ProcSetup                 = dll.NewProc("Setup")
	ProcTeardown              = dll.NewProc("Teardown")
	ProcSetHighlightRectangle = dll.NewProc("SetHighlightRectangle")
	ProcSetText               = dll.NewProc("SetText")
)
